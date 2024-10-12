
#include <snail/common/thread.hpp>

#if defined(_WIN32)
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#    include <processthreadsapi.h>
#    include <utf8/cpp17.h>
#else
#    include <pthread.h>
#endif

void snail::common::set_thread_name(const std::string& name)
{
#if defined(_WIN32)
    const auto name_u16 = utf8::utf8to16(name);
    SetThreadDescription(GetCurrentThread(), reinterpret_cast<const wchar_t*>(name_u16.data()));
#elif defined(__linux__)
    ::pthread_setname_np(::pthread_self(), name.c_str());
#elif defined(__APPLE__)
    ::pthread_setname_np(name.c_str());
#endif
}
