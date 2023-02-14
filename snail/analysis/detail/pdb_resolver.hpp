#pragma once

#include <cstdint>

#include <memory>
#include <string>
#include <unordered_map>

#ifdef SNAIL_HAS_LLVM

namespace llvm::pdb {

class IPDBSession;

} // namespace llvm::pdb

#endif // SNAIL_HAS_LLVM

namespace snail::analysis::detail {

class pdb_resolver
{
public:
    using process_id_t          = std::uint32_t;
    using timestamp_t           = std::uint64_t;
    using instruction_pointer_t = std::uint64_t;

    pdb_resolver();
    ~pdb_resolver();

    struct symbol_info;
    struct module_info;

    const symbol_info& make_generic_symbol(instruction_pointer_t address);

    const symbol_info& make_generic_symbol(const module_info& module, instruction_pointer_t address);

    const symbol_info& resolve_symbol(const module_info& module, instruction_pointer_t address);

private:
    struct module_key
    {
        process_id_t process_id;
        timestamp_t  load_timestamp;

        bool operator==(const module_key& other) const;
    };

    struct symbol_key
    {
        pdb_resolver::module_key module_key;
        instruction_pointer_t    address;

        bool operator==(const symbol_key& other) const;
    };

    struct module_key_hasher
    {
        std::size_t operator()(const module_key& key) const;

    private:
        std::hash<process_id_t> process_id_hasher;
        std::hash<timestamp_t>  timestamp_hasher;
    };

    struct symbol_key_hasher
    {
        std::size_t operator()(const symbol_key& key) const;

    private:
        pdb_resolver::module_key_hasher  module_key_hasher;
        std::hash<instruction_pointer_t> address_hasher;
    };

#ifdef SNAIL_HAS_LLVM
    llvm::pdb::IPDBSession* get_pdb_session(const module_info& module);

    std::unordered_map<module_key, std::unique_ptr<llvm::pdb::IPDBSession>, module_key_hasher> pdb_session_cache;
#endif // SNAIL_HAS_LLVM

    std::unordered_map<symbol_key, symbol_info, symbol_key_hasher> symbol_cache;
};

struct pdb_resolver::symbol_info
{
    std::string name;
    bool        is_generic;

    std::string file_path;

    std::size_t function_line_number;
    std::size_t instruction_line_number;
};

struct pdb_resolver::module_info
{
    std::string_view      image_filename;
    instruction_pointer_t image_base;

    process_id_t process_id;
    timestamp_t  load_timestamp;
};

} // namespace snail::analysis::detail
