#include <filesystem>
#include <optional>

namespace snail::detail::tests {

void parse_command_line(int argc, char* argv[]);

std::optional<std::filesystem::path> get_root_dir();

} // namespace snail::detail::tests
