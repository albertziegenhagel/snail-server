#include <snail/analysis/detail/pdb_resolver.hpp>

#include <cassert>
#include <filesystem>
#include <format>
#include <iostream>
#include <span>

#ifdef SNAIL_HAS_LLVM
#    include <llvm/DebugInfo/PDB/IPDBLineNumber.h>
#    include <llvm/DebugInfo/PDB/IPDBSession.h>
#    include <llvm/DebugInfo/PDB/IPDBSourceFile.h>
#    include <llvm/DebugInfo/PDB/Native/InfoStream.h>
#    include <llvm/DebugInfo/PDB/Native/PDBFile.h>
#    include <llvm/DebugInfo/PDB/PDB.h>
#    include <llvm/DebugInfo/PDB/PDBSymbolFunc.h>
#    include <llvm/DebugInfo/PDB/PDBSymbolPublicSymbol.h>
#    include <llvm/DebugInfo/PDB/PDBSymbolTypeFunctionSig.h>
#    include <llvm/DebugInfo/PDB/PDBTypes.h>
#    include <llvm/Demangle/Demangle.h>
#    include <llvm/Object/Binary.h>
#    include <llvm/Object/COFF.h>
#endif

#include <snail/common/cast.hpp>
#include <snail/common/hash_combine.hpp>
#include <snail/common/path.hpp>
#include <snail/common/system.hpp>

#include <snail/etl/parser/utility.hpp>

#include <snail/analysis/path_map.hpp>

#include <snail/analysis/detail/download.hpp>

using namespace snail;
using namespace snail::analysis;
using namespace snail::analysis::detail;

namespace {

struct module_filter_options
{
    enum class type
    {
        all_but_excluded,
        none_but_included
    };

    std::vector<std::string> filter_list;
};

#ifdef SNAIL_HAS_LLVM
std::string extract_symbol_function_name(const llvm::pdb::PDBSymbolFunc*         pdb_function_symbol,
                                         const llvm::pdb::PDBSymbolPublicSymbol* pdb_public_symbol)
{
    if(pdb_public_symbol != nullptr &&
       (pdb_function_symbol == nullptr || (pdb_public_symbol->getVirtualAddress() == pdb_function_symbol->getVirtualAddress())))
    {
        return llvm::demangle(pdb_public_symbol->getName());
    }

    // We could not find a public symbol. Fall-back to the regular function name
    return pdb_function_symbol->getName();
}

enum class check_pdb_result
{
    valid,
    not_a_pdb,
    invalid_pdb,
    incorrect_guid
};

check_pdb_result check_pdb(const std::filesystem::path&           pdb_path,
                           const std::optional<detail::pdb_info>& expected_pdb_info)
{
    // First, check whether the file exists at all
    if(!std::filesystem::is_regular_file(pdb_path)) return check_pdb_result::not_a_pdb;

    const auto pdb_path_str = pdb_path.string(); // FIXME: needs to be UTF-8

    // Then, check whether the file magic says it's a PDB file
    llvm::file_magic magic;
    const auto       magic_error = llvm::identify_magic(pdb_path_str, magic);
    if(magic_error || magic != llvm::file_magic::pdb) return check_pdb_result::not_a_pdb;

    // If we do not have a valid expected signature, this is all we could do
    if(expected_pdb_info == std::nullopt) return check_pdb_result::valid;

    // Now, extract the GUID from the PDB and check whether it matches the the expected GUID.

    auto buffer = llvm::MemoryBuffer::getFile(pdb_path_str, false, false);
    if(!buffer) return check_pdb_result::invalid_pdb;

    auto stream = std::make_unique<llvm::MemoryBufferByteStream>(std::move(buffer.get()),
#    if LLVM_VERSION_MAJOR >= 18
                                                                 llvm::endianness::little
#    else
                                                                 llvm::support::little
#    endif
    );

    llvm::BumpPtrAllocator              allocator;
    std::unique_ptr<llvm::pdb::PDBFile> file;
    try
    {
        file = std::make_unique<llvm::pdb::PDBFile>(pdb_path_str, std::move(stream), allocator);
    }
    catch(const std::exception&)
    {
        return check_pdb_result::invalid_pdb;
    }
    if(auto error = file->parseFileHeaders())
    {
        llvm::consumeError(std::move(error));
        return check_pdb_result::invalid_pdb;
    }
    if(auto error = file->parseStreamData())
    {
        llvm::consumeError(std::move(error));
        return check_pdb_result::invalid_pdb;
    }

    auto info_stream = file->getPDBInfoStream();
    if(!info_stream)
    {
        llvm::consumeError(info_stream.takeError());
        return check_pdb_result::invalid_pdb;
    }

    const auto llvm_guid  = info_stream->getGuid();
    const auto guid_bytes = std::as_bytes(std::span(llvm_guid.Guid));
    const auto guid       = etl::parser::guid_view(guid_bytes).instantiate(); // Assumes the signature is stored in little endian byte order!

    if(guid != expected_pdb_info->guid) return check_pdb_result::incorrect_guid;

    return check_pdb_result::valid;
}

std::optional<detail::pdb_info> try_get_pdb_info_from_module(const std::string& module_path,
                                                             std::uint32_t      checksum)
{
    auto binary_file = llvm::object::createBinary(module_path);
    if(!binary_file)
    {
        llvm::consumeError(binary_file.takeError());
        return std::nullopt;
    }

    const auto* coeff_file = llvm::dyn_cast<llvm::object::COFFObjectFile>(binary_file->getBinary());
    if(!coeff_file) return std::nullopt;

    const auto* header_plus = coeff_file->getPE32PlusHeader();
    const auto* header      = coeff_file->getPE32Header();

    const std::uint32_t image_checksum = header_plus ? header_plus->CheckSum : (header ? header->CheckSum : 0);

    // If the checksum does not match, this binary is not the one we expect and hence the PDB
    // info we would extract, would probably not match either.
    if(image_checksum != checksum) return std::nullopt;

    llvm::StringRef                  pdb_path_ref;
    const llvm::codeview::DebugInfo* llvm_pdb_info = nullptr;
    if(auto error = coeff_file->getDebugPDBInfo(llvm_pdb_info, pdb_path_ref))
    {
        llvm::consumeError(std::move(error));
        return std::nullopt;
    }

    if(llvm_pdb_info == nullptr) return std::nullopt;

    if(llvm_pdb_info->Signature.CVSignature != llvm::OMF::Signature::PDB70) return std::nullopt;

    const auto signature_bytes = std::as_bytes(std::span(llvm_pdb_info->PDB70.Signature));
    return detail::pdb_info{
        .pdb_name = std::string(pdb_path_ref),
        .guid     = etl::parser::guid_view(signature_bytes).instantiate(),
        .age      = llvm_pdb_info->PDB70.Age};
}

std::optional<std::filesystem::path> find_or_retrieve_pdb(const std::filesystem::path&           module_path,
                                                          const std::optional<detail::pdb_info>& pdb_info,
                                                          const pdb_symbol_find_options&         options,
                                                          const path_map&                        module_path_map)
{
    std::filesystem::path pdb_name;

    if(pdb_info == std::nullopt)
    {
        // If we do not have any pdb-info, we guess the PDB name by the module name.
        pdb_name = module_path.filename();
        pdb_name.replace_extension(".pdb");
    }
    else
    {
        auto pdb_ref_path_str = pdb_info->pdb_name;
        module_path_map.try_apply(pdb_ref_path_str);

        const auto pdb_ref_path = common::path_from_utf8(pdb_ref_path_str);

        // First check the reference path given in the module, if it is an absolute path
        if(pdb_ref_path.is_absolute())
        {
            if(check_pdb(pdb_ref_path, pdb_info) == check_pdb_result::valid) return pdb_ref_path;
        }

        pdb_name = pdb_ref_path.filename();

#    ifndef _WIN32
        // If we are on any Unix based system but the PDB did include an absolute Windows path
        // the `.filename()` call above would not extract the filename correctly.
        // To workaround this, we apply heuristics to detect that case. If the filename is the same
        // as the complete path and the path includes a colon, we consider the path an
        // absolute Windows path and extract the filename es everything after the final `\`.
        if(pdb_name == pdb_ref_path && pdb_ref_path_str.find(':') != std::string::npos)
        {
            const auto last_sep_pos = pdb_ref_path_str.find_last_of('\\');
            if(last_sep_pos != std::string::npos)
            {
                pdb_name = common::path_from_utf8(pdb_ref_path_str.substr(last_sep_pos + 1));
            }
        }
#    endif
    }

    // Check next to the module
    const auto pdb_next_to_module = module_path.parent_path() / pdb_name;
    if(check_pdb(pdb_next_to_module, pdb_info) == check_pdb_result::valid) return pdb_next_to_module;

    // Check in explicitly given search directories
    for(const auto& search_dir : options.search_dirs_)
    {
        const auto pdb_path = search_dir / pdb_name;
        if(check_pdb(pdb_path, pdb_info) == check_pdb_result::valid) return pdb_path;
    }

    // If we couldn't find it locally and we don't have PDB info, this is all we could do.
    if(pdb_info == std::nullopt) return std::nullopt;

    // If we have PDB info, check if the PDB is already in the cache directory or
    // try to retrieve it from symbol servers

    const auto guid_str = pdb_info->guid.to_string();

    const auto pdb_cache_path = options.symbol_cache_dir_ / pdb_name / (guid_str + std::to_string(pdb_info->age)) / pdb_name;

    // Check for cache hit
    if(check_pdb(pdb_cache_path, pdb_info) == check_pdb_result::valid) return pdb_cache_path;
    else std::filesystem::remove(pdb_cache_path);

    // Give up if we do not have any symbol servers, before creating the cache directory
    if(options.symbol_server_urls_.empty()) return std::nullopt;

    // Make sure cache directory exists
    if(!std::filesystem::exists(pdb_cache_path.parent_path()))
    {
        std::filesystem::create_directories(pdb_cache_path.parent_path());
    }

    // Try all symbol servers
    // FIXME: needs to be UTF-8
    const auto pdb_url_suffix = std::format("{0}/{1}{2}/{0}", pdb_name.string(), guid_str, (int)pdb_info->age);
    for(const auto& server_url : options.symbol_server_urls_)
    {
        const auto url = std::format("{0}/{1}", server_url, pdb_url_suffix);

        try
        {
            if(try_download_file(url, pdb_cache_path))
            {
                if(check_pdb(pdb_cache_path, pdb_info) == check_pdb_result::valid) return pdb_cache_path;
                else std::filesystem::remove(pdb_cache_path);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    // We failed to find or retrieve the PDB file.
    return std::nullopt;
}

#endif // SNAIL_HAS_LLVM

} // namespace

pdb_resolver::pdb_resolver(pdb_symbol_find_options find_options,
                           path_map                module_path_map,
                           filter_options          filter,
                           bool                    use_dia_sdk) :
    find_options_(std::move(find_options)),
    module_path_map_(std::move(module_path_map)),
    filter_(std::move(filter)),
    use_dia_sdk_(use_dia_sdk)
{
#ifndef SNAIL_HAS_LLVM
    (void)use_dia_sdk_;
#endif
}

pdb_resolver::~pdb_resolver() = default;

const pdb_resolver::symbol_info& pdb_resolver::make_generic_symbol(instruction_pointer_t address)
{
    const auto key = symbol_key{
        .module_key = module_key{
                                 .process_id     = 0,
                                 .load_timestamp = 0},
        .address = address
    };
    auto iter = symbol_cache_.find(key);
    if(iter != symbol_cache_.end()) return iter->second;

    const auto new_symbol = symbol_info{
        .name                    = std::format("{:#018x}", address),
        .is_generic              = true,
        .file_path               = {},
        .function_line_number    = {},
        .instruction_line_number = {},
    };

    const auto [new_iter, _] = symbol_cache_.emplace(key, new_symbol);
    return new_iter->second;
}

const pdb_resolver::symbol_info& pdb_resolver::make_generic_symbol(const module_info& module, instruction_pointer_t address)
{
    const auto key = symbol_key{
        .module_key = module_key{
                                 .process_id     = module.process_id,
                                 .load_timestamp = module.load_timestamp},
        .address = address
    };
    auto iter = symbol_cache_.find(key);
    if(iter != symbol_cache_.end()) return iter->second;

    auto delimiter_pos = module.image_filename.find_last_of('\\');
    if(delimiter_pos == std::u16string::npos) delimiter_pos = module.image_filename.find_last_of("//");

    const auto filename = delimiter_pos == std::u16string::npos ? module.image_filename : module.image_filename.substr(delimiter_pos + 1);

    const auto new_symbol = symbol_info{
        .name                    = std::format("{}!{:#018x}", filename, address),
        .is_generic              = true,
        .file_path               = {},
        .function_line_number    = {},
        .instruction_line_number = {},
    };

    const auto [new_iter, inserted] = symbol_cache_.emplace(key, new_symbol);
    assert(inserted);
    return new_iter->second;
}

const pdb_resolver::symbol_info& pdb_resolver::resolve_symbol(const module_info& module, instruction_pointer_t address)
{
    const auto key = symbol_key{
        .module_key = module_key{
                                 .process_id     = module.process_id,
                                 .load_timestamp = module.load_timestamp},
        .address = address
    };
    auto iter = symbol_cache_.find(key);
    if(iter != symbol_cache_.end()) return iter->second;

#ifdef SNAIL_HAS_LLVM
    auto* const pdb_session = get_pdb_session(module);
    if(pdb_session == nullptr) return make_generic_symbol(module, address);

    const auto relative_address        = common::narrow_cast<std::uint32_t>(address - module.image_base);
    const auto pdb_function_symbol_ptr = pdb_session->findSymbolByRVA(relative_address, llvm::pdb::PDB_SymType::Function);
    const auto pdb_public_symbol_ptr   = pdb_session->findSymbolByRVA(relative_address, llvm::pdb::PDB_SymType::PublicSymbol);

    const auto* const pdb_function_symbol = llvm::dyn_cast_if_present<llvm::pdb::PDBSymbolFunc>(pdb_function_symbol_ptr.get());
    const auto* const pdb_public_symbol   = llvm::dyn_cast_if_present<llvm::pdb::PDBSymbolPublicSymbol>(pdb_public_symbol_ptr.get());

    if(pdb_function_symbol == nullptr && pdb_public_symbol == nullptr)
    {
        return make_generic_symbol(module, address);
    }

    auto new_symbol = symbol_info{
        .name                    = extract_symbol_function_name(pdb_function_symbol, pdb_public_symbol),
        .is_generic              = false,
        .file_path               = {},
        .function_line_number    = {},
        .instruction_line_number = {},
    };

    if(pdb_function_symbol != nullptr)
    {
        const auto function_line_numbers = pdb_function_symbol->getLineNumbers();
        if(function_line_numbers != nullptr && function_line_numbers->getChildCount() > 0)
        {
            auto line_info = function_line_numbers->getNext();
            assert(line_info != nullptr);

            auto source_file = pdb_session->getSourceFileById(line_info->getSourceFileId());

            new_symbol.file_path            = source_file->getFileName();
            new_symbol.function_line_number = line_info->getLineNumber();
        }

        const auto length       = pdb_function_symbol->getLength();
        const auto line_numbers = pdb_session->findLineNumbersByRVA(common::narrow_cast<std::uint32_t>(relative_address), common::narrow_cast<std::uint32_t>(length));

        if(line_numbers != nullptr && line_numbers->getChildCount() > 0)
        {
            auto line_info = line_numbers->getNext();
            assert(line_info != nullptr);

            auto source_file = pdb_session->getSourceFileById(line_info->getSourceFileId());

            assert(new_symbol.file_path.empty() || new_symbol.file_path == source_file->getFileName());
            new_symbol.file_path               = source_file->getFileName();
            new_symbol.instruction_line_number = line_info->getLineNumber();
        }
    }

    const auto [new_iter, inserted] = symbol_cache_.emplace(key, std::move(new_symbol));
    assert(inserted);
    return new_iter->second;
#else  // SNAIL_HAS_LLVM
    return make_generic_symbol(module, address);
#endif // SNAIL_HAS_LLVM
}

#ifdef SNAIL_HAS_LLVM
llvm::pdb::IPDBSession* pdb_resolver::get_pdb_session(const module_info& module)
{
    const auto key = module_key{
        .process_id     = module.process_id,
        .load_timestamp = module.load_timestamp};

    auto iter = pdb_session_cache_.find(key);
    if(iter != pdb_session_cache_.end()) return iter->second.get();

    auto& new_pdb_session = pdb_session_cache_[key];

    auto module_path = std::string(module.image_filename);
    module_path_map_.try_apply(module_path);

    if(!filter_.check(module_path))
    {
        std::cout << "Skip loading PDB symbols for " << module.image_filename << std::endl;
        return new_pdb_session.get();
    }

    const auto pdb_info = module.pdb_info ?
                              module.pdb_info :
                              try_get_pdb_info_from_module(module_path, module.checksum);

    const auto pdb_path = find_or_retrieve_pdb(module_path,
                                               pdb_info,
                                               find_options_,
                                               module_path_map_);

    if(pdb_path == std::nullopt)
    {
        std::cout << "Failed to load PDB for " << module.image_filename << std::endl;
        return new_pdb_session.get();
    }

    const auto reader_type = use_dia_sdk_ && platform_supports_dia_sdk ? llvm::pdb::PDB_ReaderType::DIA : llvm::pdb::PDB_ReaderType::Native;

    if(auto error = llvm::pdb::loadDataForPDB(reader_type, pdb_path->string(), new_pdb_session))
    {
        llvm::consumeError(std::move(error));
        std::cout << "Failed to load PDB for " << module.image_filename << std::endl;
        return new_pdb_session.get();
    }
    else
    {
        std::cout << "Loaded PDB for " << module.image_filename << " from " << pdb_path->string() << "\n";
    }

    return new_pdb_session.get();
}
#endif // SNAIL_HAS_LLVM

bool pdb_resolver::module_key::operator==(const module_key& other) const
{
    return process_id == other.process_id &&
           load_timestamp == other.load_timestamp;
}

std::size_t pdb_resolver::module_key_hasher::operator()(const module_key& key) const
{
    return common::hash_combine(process_id_hasher(key.process_id), timestamp_hasher(key.load_timestamp));
}

bool pdb_resolver::symbol_key::operator==(const symbol_key& other) const
{
    return module_key == other.module_key &&
           address == other.address;
}

std::size_t pdb_resolver::symbol_key_hasher::operator()(const symbol_key& key) const
{
    return common::hash_combine(module_key_hasher(key.module_key), address_hasher(key.address));
}
