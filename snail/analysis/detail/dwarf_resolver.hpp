#pragma once

#include <cstdint>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <snail/perf_data/build_id.hpp>

#include <snail/analysis/options.hpp>
#include <snail/analysis/path_map.hpp>

namespace snail::analysis::detail {

class dwarf_resolver
{
public:
    using process_id_t          = std::uint32_t;
    using timestamp_t           = std::uint64_t;
    using instruction_pointer_t = std::uint64_t;

    dwarf_resolver(dwarf_symbol_find_options find_options = {}, path_map module_path_map = {});
    ~dwarf_resolver();

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
        dwarf_resolver::module_key module_key;
        instruction_pointer_t      address;

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
        dwarf_resolver::module_key_hasher module_key_hasher;
        std::hash<instruction_pointer_t>  address_hasher;
    };

    dwarf_symbol_find_options find_options_;
    path_map                  module_path_map_;

#ifdef SNAIL_HAS_LLVM
    struct context_storage;

    context_storage* get_dwarf_context(const module_info& module);

    std::unordered_map<module_key, std::unique_ptr<context_storage>, module_key_hasher> dwarf_context_cache_;
#endif // SNAIL_HAS_LLVM

    std::unordered_map<symbol_key, symbol_info, symbol_key_hasher> symbol_cache_;
};

struct dwarf_resolver::symbol_info
{
    std::string name;
    bool        is_generic;

    std::string file_path;

    std::size_t function_line_number;
    std::size_t instruction_line_number;
};

struct dwarf_resolver::module_info
{
    std::string_view                   image_filename;
    std::optional<perf_data::build_id> build_id;

    instruction_pointer_t image_base;
    instruction_pointer_t page_offset;

    process_id_t process_id;
    timestamp_t  load_timestamp;
};

} // namespace snail::analysis::detail
