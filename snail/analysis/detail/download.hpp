#pragma once

#include <filesystem>
#include <string>

namespace snail::analysis::detail {

bool try_download_file(const std::string&           url,
                       const std::filesystem::path& output_path);

} // namespace snail::analysis::detail
