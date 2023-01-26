#include <snail/analysis/detail/pdb_resolver.hpp>

#include <cassert>
#include <format>
#include <iostream>

// #include <llvm/DebugInfo/PDB/IPDBLineNumber.h>
#include <llvm/DebugInfo/PDB/IPDBSession.h>
// #include <llvm/DebugInfo/PDB/IPDBSourceFile.h>
#include <llvm/DebugInfo/PDB/PDB.h>
#include <llvm/DebugInfo/PDB/PDBSymbolFunc.h>
#include <llvm/DebugInfo/PDB/PDBSymbolTypeFunctionSig.h>
#include <llvm/DebugInfo/PDB/PDBTypes.h>

#include <snail/common/cast.hpp>
#include <snail/common/hash_combine.hpp>

using namespace snail;
using namespace snail::analysis::detail;

pdb_resolver::pdb_resolver()  = default;
pdb_resolver::~pdb_resolver() = default;

const pdb_resolver::symbol_info& pdb_resolver::make_generic_symbol(instruction_pointer_t address)
{
    const auto key = symbol_key{
        .module_key = module_key{
                                 .process_id     = 0,
                                 .load_timestamp = 0},
        .address = address
    };
    auto iter = symbol_cache.find(key);
    if(iter != symbol_cache.end()) return iter->second;

    const auto new_symbol = symbol_info{
        .name       = std::format("{:#018x}", address),
        .is_generic = true};

    const auto [new_iter, _] = symbol_cache.emplace(key, new_symbol);
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
    auto iter = symbol_cache.find(key);
    if(iter != symbol_cache.end()) return iter->second;

    auto delimiter_pos = module.image_filename.find_last_of("\\");
    if(delimiter_pos == std::u16string::npos) delimiter_pos = module.image_filename.find_last_of("//");

    const auto filename = delimiter_pos == std::u16string::npos ? module.image_filename : module.image_filename.substr(delimiter_pos + 1);

    const auto new_symbol = symbol_info{
        .name       = std::format("{}!{:#018x}", filename, address),
        .is_generic = true};

    const auto [new_iter, inserted] = symbol_cache.emplace(key, new_symbol);
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
    auto iter = symbol_cache.find(key);
    if(iter != symbol_cache.end()) return iter->second;

    auto* const pdb_session = get_pdb_session(module);
    if(pdb_session == nullptr) return make_generic_symbol(module, address);

    const auto relative_address = address - module.image_base;
    const auto pdb_symbol       = pdb_session->findSymbolByRVA(common::narrow_cast<std::uint32_t>(relative_address), llvm::pdb::PDB_SymType::Function);

    if(pdb_symbol == nullptr) return make_generic_symbol(module, address);

    const auto* const pdb_function_symbol = static_cast<llvm::pdb::PDBSymbolFunc*>(pdb_symbol.get());

    const auto new_symbol = symbol_info{
        .name       = pdb_function_symbol->getUndecoratedName(),
        .is_generic = false};

    const auto [new_iter, inserted] = symbol_cache.emplace(key, new_symbol);
    assert(inserted);
    return new_iter->second;
}

llvm::pdb::IPDBSession* pdb_resolver::get_pdb_session(const module_info& module)
{
    const auto key = module_key{
        .process_id     = module.process_id,
        .load_timestamp = module.load_timestamp};

    auto iter = pdb_session_cache.find(key);
    if(iter != pdb_session_cache.end()) return iter->second.get();

    auto& new_pdb_session = pdb_session_cache[key];

    const auto filename = std::string(module.image_filename);

    auto error = llvm::pdb::loadDataForEXE(llvm::pdb::PDB_ReaderType::DIA, filename, new_pdb_session);
    if(error)
    {
        llvm::consumeError(std::move(error));
        std::cout << "Failed to load PDB for " << filename << std::endl;
    }
    else
    {
        std::cout << "Loaded PDB for " << filename << "\n";
    }

    return new_pdb_session.get();
}

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
