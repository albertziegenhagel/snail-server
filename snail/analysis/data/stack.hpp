#pragma once

#include <string_view>

#include <snail/common/types.hpp>

namespace snail::analysis {

struct stack_frame
{
    common::instruction_pointer_t instruction_pointer;

    std::string_view symbol_name;
    std::string_view module_name;
};

} // namespace snail::analysis
