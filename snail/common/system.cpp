
#include <snail/common/system.hpp>

#ifdef _WIN32
#    include <ShlObj.h>
#    include <stdlib.h>
#else
#    include <cstdlib>
#endif

#include <utf8/cpp17.h>

using namespace snail::common;

std::filesystem::path snail::common::get_temp_dir() noexcept
{
    std::error_code ec;

    auto result = std::filesystem::temp_directory_path(ec);
    if(!ec)
    {
        return result;
    }

    // Make sure we always return _some_ path...
#ifdef _WIN32
    return "C:\\TEMP";
#else
    return "/tmp";
#endif
}

std::optional<std::filesystem::path> snail::common::get_home_dir() noexcept
{
#ifdef _WIN32
    PWSTR      buffer = nullptr;
    const auto result = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &buffer);

    std::unique_ptr<wchar_t, decltype(CoTaskMemFree)*> buffer_ptr(buffer, CoTaskMemFree);
    if(result != S_OK) return std::nullopt;
    return std::filesystem::path(buffer_ptr.get());
#else
    const auto* const path = std::getenv("HOME");
    if(path == nullptr) return std::nullopt;
    return path;
#endif
}

std::optional<std::string> snail::common::get_env_var(const std::string& name) noexcept
{
#ifdef _WIN32
    static_assert(sizeof(wchar_t) == sizeof(char16_t));

    const auto  name_u16   = utf8::utf8to16(name);
    wchar_t*    buffer     = nullptr;
    std::size_t result_len = 0;
    if(_wdupenv_s(&buffer, &result_len, reinterpret_cast<const wchar_t*>(name_u16.data())) != 0 || buffer == nullptr)
    {
        return std::nullopt;
    }
    std::unique_ptr<wchar_t, decltype(free)*> buffer_ptr(buffer, free);
    return utf8::utf16to8(std::u16string_view(reinterpret_cast<const char16_t*>(buffer_ptr.get()), result_len));
#else
    const auto* result = std::getenv(name.c_str());
    if(result == nullptr) return std::nullopt;
    return std::string(result);
#endif
}
