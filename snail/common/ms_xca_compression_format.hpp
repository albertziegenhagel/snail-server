
#pragma once

#include <cstdint>

namespace snail::common {

// NOTE: the values in this enum need to match the ones from
//       Microsoft's COMPRESSION_FORMAT_* macros.
enum class ms_xca_compression_format : std::uint16_t
{
    none        = 0,
    lznt1       = 2,
    xpress      = 3,
    xpress_huff = 4,
};

} // namespace snail::common
