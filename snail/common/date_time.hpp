#pragma once

#include <chrono>

namespace snail::common {

using nt_duration = std::chrono::duration<std::int64_t, std::ratio<1, 10'000'000>>; // 100 nanosecond intervals
using nt_sys_time = std::chrono::sys_time<nt_duration>;

inline constexpr nt_sys_time from_nt_timestamp(nt_duration timestamp)
{
    // 1 Jan 1970 to 1 Jan 1601 in 100 nanosecond intervals
    constexpr auto nt_to_unix_epoch = nt_duration(-116444736000000000LL);

    // NOTE: Not sure whether we need to correct for leap seconds here.
    //       sys_time has to be without leap seconds and it is unknown whether the nt_timestamps do
    //       include leap seconds or not. From reading the MS STL source code it seems that
    //       leap seconds are (usually) included for dates > 2018-06-01 and not included otherwise.
    //       So, for dates > 2018-06-01 we do actually have a utf_time and not at sys_time.

    return nt_sys_time(timestamp + nt_to_unix_epoch);
}

} // namespace snail::common