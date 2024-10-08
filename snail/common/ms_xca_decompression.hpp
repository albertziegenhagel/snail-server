
#pragma once

#include <bit>
#include <span>

#include <snail/common/ms_xca_compression_format.hpp>

namespace snail::common {

// This is supposed to be a replacement for
//   RtlDecompressBufferEx
std::size_t ms_xca_decompress(std::span<const std::byte> input,
                              std::span<std::byte>       output,
                              ms_xca_compression_format  format);

} // namespace snail::common
