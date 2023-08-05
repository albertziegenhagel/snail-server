#pragma once

#include <string_view>

namespace snail::analysis {

struct stack_frame
{
    std::string_view symbol_name;
    std::string_view module_name;

    std::string_view file_path;

    std::size_t function_line_number;
    std::size_t instruction_line_number;
};

} // namespace snail::analysis
