#include <snail/analysis/detail/dwarf_resolver.hpp>

#include <cassert>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>

#ifdef SNAIL_HAS_LLVM
#    include <llvm/DebugInfo/DWARF/DWARFContext.h>
#    include <llvm/Demangle/Demangle.h>
#    include <llvm/Object/ELFObjectFile.h>
#endif // SNAIL_HAS_LLVM

#include <snail/common/cast.hpp>
#include <snail/common/hash_combine.hpp>

#include <snail/analysis/detail/download.hpp>

using namespace snail;
using namespace snail::analysis;
using namespace snail::analysis::detail;

namespace {

std::optional<std::filesystem::path> find_or_retrieve_binary(const std::filesystem::path&              input_binary_path,
                                                             const std::optional<perf_data::build_id>& build_id,
                                                             const dwarf_symbol_find_options&          options)
{
    // First check the given input path
    if(std::filesystem::is_regular_file(input_binary_path)) return input_binary_path;

    const auto binary_name = input_binary_path.filename();

    // Check in explicitly given search directories
    for(const auto& search_dir : options.search_dirs_)
    {
        const auto binary_path = search_dir / binary_name;
        if(std::filesystem::is_regular_file(binary_path)) return binary_path;
    }

    // If we do not have a build ID, this was all we can do
    if(!build_id) return std::nullopt;

    const auto build_id_str = build_id->to_string();

    // Look up the debuginfod cache
    const auto binary_cache_path = options.debuginfod_cache_dir_ / build_id_str / "executable";
    if(std::filesystem::is_regular_file(binary_cache_path)) return binary_cache_path;

    // Make sure the cache directory exists
    if(!std::filesystem::exists(binary_cache_path.parent_path()))
    {
        std::filesystem::create_directories(binary_cache_path.parent_path());
    }

    // Try to download the file from the given debuginfod servers (if any)
    for(const auto& server_url : options.debuginfod_urls_)
    {
        const auto url = std::format("{}/buildid/{}/executable", server_url, build_id_str);

        try
        {
            if(try_download_file(url, binary_cache_path))
            {
                return binary_cache_path;
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    return std::nullopt;
}

} // namespace

dwarf_resolver::dwarf_resolver(dwarf_symbol_find_options find_options,
                               path_map                  module_path_map) :
    find_options_(std::move(find_options)),
    module_path_map_(std::move(module_path_map))
{}

dwarf_resolver::~dwarf_resolver() = default;

const dwarf_resolver::symbol_info& dwarf_resolver::make_generic_symbol(instruction_pointer_t address)
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

const dwarf_resolver::symbol_info& dwarf_resolver::make_generic_symbol(const module_info& module, instruction_pointer_t address)
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

#ifdef SNAIL_HAS_LLVM
struct dwarf_resolver::context_storage
{
    std::unique_ptr<llvm::object::Binary> binary;
    std::unique_ptr<llvm::MemoryBuffer>   memory;
    const llvm::object::ObjectFile*       object_file;
    std::unique_ptr<llvm::DWARFContext>   context;
};
#endif // SNAIL_HAS_LLVM

const dwarf_resolver::symbol_info& dwarf_resolver::resolve_symbol(const module_info&    module,
                                                                  instruction_pointer_t address)
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
    const auto relative_address = address - module.image_base + module.page_offset;

    auto* const dwarf_context = get_dwarf_context(module);
    if(dwarf_context == nullptr) return make_generic_symbol(module, address);

    llvm::object::SectionedAddress sectioned_address;

    const auto* const elf_object_file = llvm::dyn_cast<llvm::object::ELFObjectFileBase>(dwarf_context->object_file);
    if(elf_object_file != nullptr)
    {
        for(auto section : elf_object_file->sections())
        {
            if(!section.isText() || section.isVirtual())
                continue;

            const auto elf_section = llvm::object::ELFSectionRef(section);

            if(relative_address >= elf_section.getOffset() &&
               relative_address < elf_section.getOffset() + elf_section.getSize())
            {
                sectioned_address.Address      = relative_address - elf_section.getOffset() + elf_section.getAddress();
                sectioned_address.SectionIndex = section.getIndex();
                break;
            }
        }
    }

    auto line_info_specifier = llvm::DILineInfoSpecifier(llvm::DILineInfoSpecifier::FileLineInfoKind::AbsoluteFilePath, llvm::DILineInfoSpecifier::FunctionNameKind::LinkageName);

    auto inlining_info = dwarf_context->context->getInliningInfoForAddress(sectioned_address, line_info_specifier);

    const auto number_of_inlined_frames = inlining_info.getNumberOfFrames();
    if(number_of_inlined_frames == 0)
    {
        return make_generic_symbol(module, address);
    }

    auto line_info = inlining_info.getFrame(number_of_inlined_frames - 1);

    if(line_info.FunctionName == llvm::DILineInfo::BadString)
    {
        return make_generic_symbol(module, address);
    }

    auto new_symbol = symbol_info{
        .name                    = {},
        .is_generic              = false,
        .file_path               = line_info.FileName,
        .function_line_number    = line_info.StartLine,
        .instruction_line_number = line_info.Line - 1 // we want line numbers to be zero based
    };

    if(!llvm::nonMicrosoftDemangle(line_info.FunctionName.c_str(), new_symbol.name))
    {
        new_symbol.name = line_info.FunctionName;
    }

    const auto [new_iter, inserted] = symbol_cache_.emplace(key, std::move(new_symbol));
    assert(inserted);
    return new_iter->second;
#else  // SNAIL_HAS_LLVM
    return make_generic_symbol(module, address);
#endif // SNAIL_HAS_LLVM
}

#ifdef SNAIL_HAS_LLVM
dwarf_resolver::context_storage* dwarf_resolver::get_dwarf_context(const module_info& module)
{
    const auto key = module_key{
        .process_id     = module.process_id,
        .load_timestamp = module.load_timestamp};

    auto iter = dwarf_context_cache_.find(key);
    if(iter != dwarf_context_cache_.end()) return iter->second.get();

    auto& new_context_storage = dwarf_context_cache_[key];

    auto input_binary_path = std::string(module.image_filename);
    module_path_map_.try_apply(input_binary_path);

    const auto binary_path = find_or_retrieve_binary(input_binary_path, module.build_id, find_options_);

    if(!binary_path)
    {
        std::cout << "Failed to load DWARF debug info for " << module.image_filename << std::endl;
        return nullptr;
    }

    auto binary_file = llvm::object::createBinary(binary_path->string());
    if(!binary_file)
    {
        llvm::consumeError(binary_file.takeError());
        std::cout << "Failed to load DWARF debug info for " << module.image_filename << std::endl;
        return nullptr;
    }

    if(!binary_file->getBinary()->isObject())
    {
        std::cout << "Failed to load DWARF debug info for " << module.image_filename << std::endl;
        return nullptr;
    }

    const auto* const object_file = llvm::cast<llvm::object::ObjectFile>(binary_file->getBinary());

    auto context = llvm::DWARFContext::create(*object_file);

    if(context == nullptr)
    {
        std::cout << "Failed to load DWARF debug info for " << module.image_filename << std::endl;
        return nullptr;
    }

    new_context_storage = std::make_unique<context_storage>();

    auto binary_pair            = binary_file->takeBinary();
    new_context_storage->binary = std::move(binary_pair.first);
    new_context_storage->memory = std::move(binary_pair.second);

    new_context_storage->object_file = object_file;
    new_context_storage->context     = std::move(context);

    std::cout << "Loaded DWARF debug info for " << module.image_filename << " from " << binary_path->string() << "\n";

    return new_context_storage.get();
}
#endif // SNAIL_HAS_LLVM

bool dwarf_resolver::module_key::operator==(const module_key& other) const
{
    return process_id == other.process_id &&
           load_timestamp == other.load_timestamp;
}

std::size_t dwarf_resolver::module_key_hasher::operator()(const module_key& key) const
{
    return common::hash_combine(process_id_hasher(key.process_id), timestamp_hasher(key.load_timestamp));
}

bool dwarf_resolver::symbol_key::operator==(const symbol_key& other) const
{
    return module_key == other.module_key &&
           address == other.address;
}

std::size_t dwarf_resolver::symbol_key_hasher::operator()(const symbol_key& key) const
{
    return common::hash_combine(module_key_hasher(key.module_key), address_hasher(key.address));
}
