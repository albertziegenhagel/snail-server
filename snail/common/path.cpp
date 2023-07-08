
#include <snail/common/path.hpp>

#include <string>

std::filesystem::path snail::common::path_from_utf8(std::string_view path_str)
{
    // Unfortunately, std::filesystem::u8path has been deprecated in C++20 and the following
    // is the "official" workaround until eventually the C++ committee will come up with a
    // final solution.
    // See https://github.com/cplusplus/papers/issues/1415
    std::u8string path_u8str(path_str.data(), path_str.data() + path_str.size());
    return {path_u8str};
}
