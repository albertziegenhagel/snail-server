#pragma once

#include <cstdint>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <snail/analysis/options.hpp>
#include <snail/analysis/path_map.hpp>

#include <snail/analysis/detail/pdb_info.hpp>

#ifdef SNAIL_HAS_LLVM

namespace llvm::pdb {

class IPDBSession;

} // namespace llvm::pdb

#endif // SNAIL_HAS_LLVM

namespace snail::analysis::detail {

// The Microsoft DiaSDK is only available on Windows
inline constexpr auto platform_supports_dia_sdk =
#ifdef _WIN32
    true
#else
    false
#endif
    ;

// Use the DiaSDK by default where it is supported (Windows only) since it is faster than the LLVM Native reader
inline constexpr auto default_use_dia_sdk = platform_supports_dia_sdk;

class pdb_resolver
{
public:
    using os_pid_t              = std::uint32_t;
    using timestamp_t           = std::uint64_t;
    using instruction_pointer_t = std::uint64_t;

    explicit pdb_resolver(pdb_symbol_find_options find_options    = {},
                          path_map                module_path_map = {},
                          filter_options          filter          = {},
                          bool                    use_dia_sdk     = default_use_dia_sdk);
    ~pdb_resolver();

    struct symbol_info;
    struct module_info;

    const symbol_info& make_generic_symbol(instruction_pointer_t address);

    const symbol_info& make_generic_symbol(const module_info& module, instruction_pointer_t address);

    const symbol_info& resolve_symbol(const module_info& module, instruction_pointer_t address);

private:
    struct module_key
    {
        os_pid_t    process_id;
        timestamp_t load_timestamp;

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
        std::hash<os_pid_t>    process_id_hasher;
        std::hash<timestamp_t> timestamp_hasher;
    };

    struct symbol_key_hasher
    {
        std::size_t operator()(const symbol_key& key) const;

    private:
        pdb_resolver::module_key_hasher  module_key_hasher;
        std::hash<instruction_pointer_t> address_hasher;
    };

    pdb_symbol_find_options find_options_;
    path_map                module_path_map_;
    filter_options          filter_;
    bool                    use_dia_sdk_;

#ifdef SNAIL_HAS_LLVM
    llvm::pdb::IPDBSession* get_pdb_session(const module_info& module);

    std::unordered_map<module_key, std::unique_ptr<llvm::pdb::IPDBSession>, module_key_hasher> pdb_session_cache_;
#endif // SNAIL_HAS_LLVM

    std::unordered_map<symbol_key, symbol_info, symbol_key_hasher> symbol_cache_;
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
    std::string_view                image_filename;
    instruction_pointer_t           image_base;
    std::uint32_t                   checksum;
    std::optional<detail::pdb_info> pdb_info;

    os_pid_t    process_id;
    timestamp_t load_timestamp;
};

} // namespace snail::analysis::detail
