#include "progress_printer.hpp"

#include <cmath>
#include <format>
#include <iostream>

#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif

namespace {
class cursor_visibility_guard
{
public:
    explicit cursor_visibility_guard(bool visible) :
        original_visibility_(set_console_cursor_visibility(visible))
    {}

    ~cursor_visibility_guard()
    {
        set_console_cursor_visibility(original_visibility_);
    }

    bool set_console_cursor_visibility(bool visible)
    {
#if defined(_WIN32)
        auto stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

        CONSOLE_CURSOR_INFO cursorInfo;

        GetConsoleCursorInfo(stdout_handle, &cursorInfo);
        const auto original_visibility = cursorInfo.bVisible;
        cursorInfo.bVisible            = visible;
        SetConsoleCursorInfo(stdout_handle, &cursorInfo);
        return original_visibility;
#elif defined(__linux__)
        std::cout << (visible ? "\033[?25h" : "\033[?25l");
        return true;
#else
        return true;
#endif
    }

private:
    bool original_visibility_;
};

void draw_progress_bar(double progress, unsigned int display_length = 80, std::ostream& out_stream = std::cout, bool clear = true)
{
    static constexpr unsigned int special_display_char_count = 9; // '[] xxx.x%' <- 7 characters

    const auto max_chars = display_length - special_display_char_count;

    const auto display_chars = std::min(static_cast<unsigned int>(std::round(progress * max_chars)),
                                        max_chars);
    const auto percent       = std::round(progress * 1000) / 10;

    // if(clear) out_stream << '\r'; // Go to start of current line
    cursor_visibility_guard guard(false);
    out_stream << '[';
    for(unsigned int i = 0; i < display_chars; ++i)
    {
        out_stream << '=';
    }
    for(unsigned int i = display_chars; i < max_chars; ++i)
    {
        out_stream << ' ';
    }
    out_stream << ']';

    out_stream << std::format("{:>6.1f}%", percent);
    if(clear)
    {
        out_stream << std::flush;
        out_stream << '\r'; // Go to start of current line
    }
    else
    {
        out_stream << std::endl;
    }
}

} // namespace

progress_printer::progress_printer() :
    snail::common::progress_listener(0.01) // every 1%
{}

void progress_printer::start([[maybe_unused]] std::string_view                title,
                             [[maybe_unused]] std::optional<std::string_view> message) const {}

void progress_printer::report(double                                           progress,
                              [[maybe_unused]] std::optional<std::string_view> message) const
{
    draw_progress_bar(progress);
}

void progress_printer::finish([[maybe_unused]] std::optional<std::string_view> message) const
{
    std::cout << std::endl;
}
