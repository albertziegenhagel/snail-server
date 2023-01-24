#pragma once

#include <string>

#include <snail/common/generator.hpp>

#include <snail/data/types.hpp>

namespace snail::analysis {

// struct symbol_info
// {
//     virtual ~symbol_info() = default;

//     virtual std::string_view name() const = 0;
//     virtual bool             is_generic() const = 0;
// };

struct stack_entry
{
    virtual ~stack_entry() = default;

    virtual data::instruction_pointer_t instruction_pointer() const = 0;
    virtual std::string_view symbol_name() const = 0;
};

struct sample_data
{
    virtual ~sample_data() = default;

    virtual bool has_stack() const = 0;
    virtual common::generator<const stack_entry&> reversed_stack() const = 0;
};

struct process_info
{
    virtual ~process_info() = default;

    virtual data::process_id_t process_id() const = 0;
    virtual data::timestamp_t  start_time() const = 0;

    virtual std::string_view image_name() const = 0;
};

class stack_provider
{
public:
    virtual ~stack_provider() = default;

    virtual common::generator<const process_info&> processes() const = 0;

    virtual common::generator<const sample_data&> samples(const process_info& process) const = 0;
};

} // namespace snail::analysis