#include <folders.hpp>

#include <string_view>

static std::optional<std::filesystem::path> g_snail_root_dir;

void snail::detail::tests::parse_command_line(int argc, char* argv[])
{
    using namespace std::string_view_literals;
    constexpr const auto root_dir_arg_prefix = "--snail-root-dir="sv;

    for(int i = 1; i < argc; ++i)
    {
        const auto argument = std::string_view(argv[i]);
        if(argument.starts_with(root_dir_arg_prefix))
        {
            g_snail_root_dir = argument.substr(root_dir_arg_prefix.length());
        }
    }
}

std::optional<std::filesystem::path> snail::detail::tests::get_root_dir()
{
    if(g_snail_root_dir && std::filesystem::exists(*g_snail_root_dir))
    {
        return *g_snail_root_dir;
    }
    return std::nullopt;
}
