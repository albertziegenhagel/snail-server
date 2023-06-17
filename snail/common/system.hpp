#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace snail::common {

std::filesystem::path get_temp_dir() noexcept;

std::optional<std::filesystem::path> get_home_dir() noexcept;

std::optional<std::string> get_env_var(const std::string& name) noexcept;

} // namespace snail::common
