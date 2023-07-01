
#include <snail/common/unicode.hpp>

#if _WIN32
#    include <Windows.h>
#    include <stdexcept>
#else
#    include <codecvt>
#    include <locale>
#endif

#include <snail/common/cast.hpp>

using namespace snail::common;

template<typename Utf8Char>
    requires std::is_same_v<Utf8Char, char> || std::is_same_v<Utf8Char, char8_t>
std::basic_string<Utf8Char> snail::common::utf16_to_utf8(std::u16string_view input)
{
    if(input.empty()) return {};

#if _WIN32
    // FIXME: The conversion from the char16_t string data to wchar_t in the following is
    //        probably undefined behaviour (aliasing rules), but we expect it to work anyways?
    static_assert(sizeof(char16_t) == sizeof(wchar_t));

    const auto result_size = WideCharToMultiByte(CP_UTF8, 0,
                                                 reinterpret_cast<const wchar_t*>(input.data()), narrow_cast<int>(input.size()),
                                                 nullptr, 0,
                                                 nullptr, nullptr);

    if(result_size <= 0) throw std::runtime_error("Failed to convert UTF-16 string to UTF-8");

    std::basic_string<Utf8Char> result_string(result_size, '\0');

    const auto bytes_converted = WideCharToMultiByte(CP_UTF8, 0,
                                                     reinterpret_cast<const wchar_t*>(input.data()), narrow_cast<int>(input.size()),
                                                     reinterpret_cast<char*>(result_string.data()), narrow_cast<int>(result_string.size()),
                                                     nullptr, nullptr);

    if(bytes_converted == 0) throw std::runtime_error("Failed to convert UTF-16 string to UTF-8");

    return result_string;
#else
    // FIXME: do not rely on deprecated functionality
    auto       convert       = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{};
    const auto result_string = convert.to_bytes(input.data(), input.data() + input.size());

    if constexpr(std::is_same_v<Utf8Char, char>)
    {
        return result_string;
    }
    else
    {
        // TODO: can we get rid of the copy?
        return std::basic_string<Utf8Char>(result_string.begin(), result_string.end());
    }
#endif
}

template std::basic_string<char>    snail::common::utf16_to_utf8<char>(std::u16string_view);
template std::basic_string<char8_t> snail::common::utf16_to_utf8<char8_t>(std::u16string_view);
