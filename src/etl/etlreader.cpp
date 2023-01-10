#include <cassert>

#include <stdexcept>
#include <iostream>
#include <format>
#include <unordered_map>
#include <map>
#include <array>
#include <concepts>
#include <algorithm>
#include <set>

#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>

#include <dia2.h>
#include <atlbase.h>

#include "etlreader.hpp"

using namespace perfreader;

namespace perfreader {

struct guid
{
    unsigned long  data_1;
    unsigned short data_2;
    unsigned short data_3;
    std::array<unsigned char, 8> data_4;

    bool operator==(const guid& other) const
    {
        return data_1 == other.data_1 &&
            data_2 == other.data_2 &&
            data_3 == other.data_3 &&
            data_4 == other.data_4;
    }
};

struct module_info
{
    std::uint64_t image_base;
    std::uint64_t image_size;
    std::wstring file_name;

    std::size_t stack_total_count = 0;
    std::size_t stack_self_count = 0;
    std::size_t sample_self_count = 0;

    struct address_info
    {
        std::size_t stack_total_count = 0;
        std::size_t stack_self_count = 0;
        std::size_t sample_self_count = 0;

        std::wstring function_name;
        std::wstring file_name;
        std::size_t line_number;
    };

    bool active;

    std::unordered_map<std::uint64_t, address_info> per_address_count;
    
    bool operator==(const module_info& other) const
    {
        return image_base == other.image_base &&
            image_size == other.image_size &&
            file_name == other.file_name;
    }
};

struct stack_frame
{
    std::uint64_t address;
};

struct stack_trace
{
    std::vector<stack_frame> frames;
};

struct profile_sample_data
{
    std::uint64_t instruction_pointer;
    std::uint32_t thread_id;

    std::vector<stack_trace> stacks;
};

} // namespace perfreader


// Specialize std::hash
namespace std {
template<> struct hash<perfreader::guid>
{
    size_t operator()(const perfreader::guid& guid) const noexcept {
        const std::uint64_t* p = reinterpret_cast<const std::uint64_t*>(&guid);
        std::hash<std::uint64_t> hash;
        return hash(p[0]) ^ hash(p[1]);
    }
};
} // namespace std

static std::unordered_map<perfreader::guid, std::map<std::uint8_t, std::size_t>> count_by_provider;
static std::map<std::uint32_t, std::string> known_processes;
static std::map<std::uint32_t, std::uint32_t> thread_to_process;

static std::vector<module_info> loaded_modules;

static std::size_t proc_samples;
static std::size_t proc_stacks_total;
static std::size_t proc_stacks_self;

static std::size_t mod_samples;
static std::size_t mod_stacks_total;
static std::size_t mod_stacks_self;

static std::size_t kernel_samples;


static std::size_t all_kernel_stacks;
static std::size_t non_kernel_stacks;



static std::size_t debug_count;
static std::size_t debug_count_2;

static std::map<std::uint64_t, std::vector<profile_sample_data>> profile_samples;

perfreader::guid to_internal(const GUID& guid)
{
    return perfreader::guid{guid.Data1, guid.Data2, guid.Data3, {guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]}};
}
constexpr inline auto guid_perf_info = perfreader::guid{0xce1dbfb4, 0x137e, 0x4da6, {0x87, 0xb0, 0x3f, 0x59, 0xaa, 0x10, 0x2c, 0xbc}};
constexpr inline auto guid_stack_walk = perfreader::guid{0xdef2fe46, 0x7bd6, 0x4b80, {0xbd, 0x94, 0xf5, 0x7f, 0xe2, 0x0d, 0x0c, 0xe3}};
constexpr inline auto guid_process = perfreader::guid{0x3d6fa8d0, 0xfe05, 0x11d0, {0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c}};
constexpr inline auto guid_thread = perfreader::guid{0x3d6fa8d1, 0xfe05, 0x11d0, {0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c}};
constexpr inline auto guid_image = perfreader::guid{0x2cb15d1d, 0x5fc1, 0x11d2, {0xab, 0xe1, 0x00, 0xa0, 0xc9, 0x11, 0xf5, 0x18}};


// 68fdd900-4a3e-11d1-84f40000f80464e3 EventTraceEvent: 17 -> header
// 9b79ee91-b5fd-41c0-a2434248e266e9d0 SysConfig??:     12
// 9e5f9046-43c6-4f62-ba137b19896253ff ???:   205
// 2cb15d1d-5fc1-11d2-abe100a0c911f518 ImageTask:  21208
// ce1dbfb4-137e-4da6-87b03f59aa102cbc PerfInfo:  168850 -> perfinfo
// 3d6fa8d0-fe05-11d0-9dda00c04fd7ba7c Process:     1091
// 3d6fa8d1-fe05-11d0-9dda00c04fd7ba7c Thread:     13965  -> thread
// b3e675d7-2554-4f18-830b2762732560de KernelTraceControlImageIdGuid???     85261
// def2fe46-7bd6-4b80-bd94f57fe20d0ce3 StackWalk:  55685 -> stackwalk
// 01853a65-418f-4f36-aefcdc0f1d2fd235 HWConfig/SystemConfig:     509  -> config

// 01853a65-418f-4f36-aefcdc0f1d2fd235 HWConfig
// 01853a65-418f-4f36-aefcdc0f1d2fd235 SystemConfig

void print_guid(const GUID& guid)
{
    std::cout << std::format("{:08x}-{:04x}-{:04x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

void print_guid(const perfreader::guid& guid)
{
    std::cout << std::format("{:08x}-{:04x}-{:04x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
        guid.data_1, guid.data_2, guid.data_3,
        guid.data_4[0], guid.data_4[1], guid.data_4[2], guid.data_4[3],
        guid.data_4[4], guid.data_4[5], guid.data_4[6], guid.data_4[7]);
}

// (int)0x9e814aad), unchecked((short)0x3204), unchecked((short)0x11d2), 0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39

// std::uint64_t extract_uint64(const std::byte*& data)
// {
//     // const auto result = std::uint64_t(
//     //     (data[0] << 56) |
//     //     (data[1] << 48) |
//     //     (data[2] << 40) |
//     //     (data[3] << 32) |
//     //     (data[4] << 24) |
//     //     (data[5] << 16) |
//     //     (data[6] << 8) |
//     //     (data[7] << 0));
//     // data += 8;
    
//     const auto result = (*(std::uint64_t*)data);
//     data += sizeof(std::uint64_t);
//     return result;
// }

// std::uint32_t extract_uint32(const std::byte*& data)
// {
//     // const auto result = std::uint32_t(
//     //     (data[0] << 24) |
//     //     (data[1] << 16) |
//     //     (data[2] << 8) |
//     //     (data[3] << 0));
    
//     const auto result = (*(std::uint32_t*)data);
//     data += sizeof(std::uint32_t);
//     return result;
// }

template<typename T>
// requires std::integral<T>
T extract(const std::byte*& data)
{
    const auto result = (*(T*)data);
    data += sizeof(T);
    return result;
}

std::string extract_string(const std::byte*& data)
{
    std::ptrdiff_t length = 0;
    while(*(data + length) != std::byte(0))
    {
        // FIXME: stop at max length
        ++length;
    }

    auto result = std::string((const char*)data, length);
    data += length + 1;
    return result;
}

std::wstring extract_wstring(const std::byte*& data)
{
    std::ptrdiff_t length = 0;
    while(*(data + length*2) != std::byte(0) || *(data + length*2 + 1) != std::byte(0))
    {
        // FIXME: stop at max length
        length += 1;
    }

    auto result = std::wstring((const wchar_t*)data, length);
    data += length*2 + 2;
    return result;
}

bool is_kernel_address(std::uint64_t address)
{
    // See https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/user-space-and-system-space

    // return address >= 0x80000000; // 32bit
    // return address >= 0x0000080000000000; // 64bit ??? <- this is given in the documentation
    return address >= 0x0000800000000000; // 64bit
}

void event_callback(PEVENT_RECORD event_record)
{
    const auto provider_guid = to_internal(event_record->EventHeader.ProviderId);

    // const auto context = event_record->UserContext;
    // std::cout << "Provider: ";
    // print_guid(event_record->EventHeader.ProviderId);
    // std::cout << " EventId: " << event_record->EventHeader.EventDescriptor.Id;
    // std::cout << "\n";

    const std::uint64_t process_of_interest = 17732;

    if((event_record->EventHeader.Flags & EVENT_HEADER_FLAG_CLASSIC_HEADER) == 0)
    {
        count_by_provider[provider_guid][event_record->EventHeader.EventDescriptor.Id]++;
        
        const auto* const user_data_start = static_cast<std::byte*>(event_record->UserData);
        const auto* user_data_ptr = user_data_start;

        std::vector<std::byte> rest_data(event_record->UserDataLength);

        std::copy(user_data_ptr, user_data_ptr + event_record->UserDataLength, rest_data.begin());
        
        if(event_record->EventHeader.EventDescriptor.Id == 6)
        {
            const auto x = extract<std::int32_t>(user_data_ptr);
            const auto time_qpc = extract<std::int64_t>(user_data_ptr); // time qpc
            const auto v1 = extract<std::int32_t>(user_data_ptr);
            const auto v2 = extract<std::int32_t>(user_data_ptr);
            int stop = 0;
        }
        else if(event_record->EventHeader.EventDescriptor.Id == 1)
        {
            const auto process_id = extract<std::int32_t>(user_data_ptr);
            const auto a = extract<std::int32_t>(user_data_ptr);
            const auto time_qpc = extract<std::int64_t>(user_data_ptr); // time qpc
            // const auto c = extract<std::int32_t>(user_data_ptr);
            int stop = 0;
        }
        else if(event_record->EventHeader.EventDescriptor.Id == 5)
        {
            const auto hostname = extract_wstring(user_data_ptr);
            const auto os = extract_wstring(user_data_ptr);
            const auto c = extract<std::int32_t>(user_data_ptr);

            int stop = 0;
        }

        const auto read = user_data_ptr - user_data_start;
        
        assert(read == event_record->UserDataLength);

        return;
    }

    count_by_provider[provider_guid][event_record->EventHeader.EventDescriptor.Opcode]++;

    const auto is_32_bit = (event_record->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) != 0;
    const auto is_64_bit = (event_record->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0;

    assert(is_64_bit);
    using pointer_type = std::uint64_t;

    if(event_record->EventHeader.EventDescriptor.Opcode == 0) return;

    const auto* const user_data_start = static_cast<std::byte*>(event_record->UserData);
    const auto* user_data_ptr = user_data_start;

    if(provider_guid == guid_process && (
        (event_record->EventHeader.EventDescriptor.Opcode == 1) || // Start
        (event_record->EventHeader.EventDescriptor.Opcode == 2) || // Stop
        (event_record->EventHeader.EventDescriptor.Opcode == 3) || // DCStart
        (event_record->EventHeader.EventDescriptor.Opcode == 4) //|| // DCStop
        // (event_record->EventHeader.EventDescriptor.Opcode == 39) // Defunct
        ))
    {
        const auto UniqueProcessKey = extract<pointer_type>(user_data_ptr);
        const auto ProcessId = extract<std::uint32_t>(user_data_ptr);
        const auto ParentId = extract<std::uint32_t>(user_data_ptr);
        const auto SessionId = extract<std::uint32_t>(user_data_ptr);
        const auto ExitStatus = extract<std::int32_t>(user_data_ptr);
        if(event_record->EventHeader.EventDescriptor.Version >= 3)
        {
            const auto DirectoryTableBase = extract<pointer_type>(user_data_ptr);
        }
        if(event_record->EventHeader.EventDescriptor.Version >= 4)
        {
            const auto flags = extract<std::int32_t>(user_data_ptr);
        }
        // object UserSID = extract<pointer_type>(user_data_ptr);
        
        const auto user_sid = extract<std::int32_t>(user_data_ptr);
        if(user_sid != 0)
        {
            const auto token_size = 2 * sizeof(pointer_type);
            user_data_ptr += token_size - sizeof(std::int32_t) + 1;
            const auto num_authorities = extract<std::int8_t>(user_data_ptr);
            user_data_ptr += 6 + num_authorities*4;
        }

        const auto current = (user_data_ptr - user_data_start);

        assert(current <= event_record->UserDataLength);
        const auto rest_size = event_record->UserDataLength - current;

        std::vector<std::byte> rest_data(rest_size);

        std::copy(user_data_ptr, user_data_ptr + rest_size, rest_data.begin());

        const auto ImageFileName = extract_string(user_data_ptr);
        const auto CommandLine = extract_wstring(user_data_ptr);
        
        std::wstring package_full_name;
        std::wstring application_id;
        if(event_record->EventHeader.EventDescriptor.Version >= 4)
        {
            package_full_name = extract_wstring(user_data_ptr);
            application_id = extract_wstring(user_data_ptr);

            int stop = 0;
        }

        const auto read_data = user_data_ptr - user_data_start;

        long prod_id = event_record->EventHeader.ProcessId;

        if(event_record->EventHeader.EventDescriptor.Opcode == 1 ||
            event_record->EventHeader.EventDescriptor.Opcode == 3)
        {
            known_processes[ProcessId] = ImageFileName;
        }
        else if(event_record->EventHeader.EventDescriptor.Opcode == 2 ||
            event_record->EventHeader.EventDescriptor.Opcode == 4)
        {
            const auto iter = known_processes.find(ProcessId);
            assert(iter != known_processes.end());
            assert(iter->second == ImageFileName);
            known_processes.erase(iter);
        }
        
        assert(event_record->EventHeader.EventDescriptor.Version > 4 || read_data == event_record->UserDataLength);
        assert(event_record->EventHeader.EventDescriptor.Version != 5 || read_data+8 == event_record->UserDataLength);
        assert(event_record->EventHeader.EventDescriptor.Version < 5 || read_data <= event_record->UserDataLength);
    }
    if(provider_guid == guid_thread && (
        (event_record->EventHeader.EventDescriptor.Opcode == 1) || // Start
        (event_record->EventHeader.EventDescriptor.Opcode == 2) || // Stop
        (event_record->EventHeader.EventDescriptor.Opcode == 3) || // DCStart
        (event_record->EventHeader.EventDescriptor.Opcode == 4) //|| // DCStop)
        ))
    {
        const auto ProcessId = extract<std::uint32_t>(user_data_ptr);
        const auto ThreadId = extract<std::uint32_t>(user_data_ptr);

        const auto StackBase = extract<pointer_type>(user_data_ptr);
        const auto StackLimit = extract<pointer_type>(user_data_ptr);
        const auto UserStackBase = extract<pointer_type>(user_data_ptr);
        const auto UserStackLimit = extract<pointer_type>(user_data_ptr);
        const auto StartAddr = extract<pointer_type>(user_data_ptr);
        const auto Win32StartAddr = extract<pointer_type>(user_data_ptr);
        const auto TebBase = extract<pointer_type>(user_data_ptr);

        if(event_record->EventHeader.EventDescriptor.Opcode == 1 ||
            event_record->EventHeader.EventDescriptor.Opcode == 3)
        {
            thread_to_process[ThreadId] = ProcessId;
        }
        else if(event_record->EventHeader.EventDescriptor.Opcode == 2 ||
            event_record->EventHeader.EventDescriptor.Opcode == 4)
        {
            const auto iter = thread_to_process.find(ThreadId);
            if(iter != thread_to_process.end())
            {
                // assert(iter != thread_to_process.end());
                assert(iter->second == ProcessId);
                // thread_to_process.erase(iter);
            }
        }
    }
    if(provider_guid == guid_image && (
        (event_record->EventHeader.EventDescriptor.Opcode == 2) || // Unload
        (event_record->EventHeader.EventDescriptor.Opcode == 3) || // DCStart
        (event_record->EventHeader.EventDescriptor.Opcode == 4) || // DCEnd
        (event_record->EventHeader.EventDescriptor.Opcode == 10) //|| // Load
        ))
    {
        assert(event_record->EventHeader.EventDescriptor.Version >= 2);

        module_info new_info;

        new_info.image_base = extract<pointer_type>(user_data_ptr);
        new_info.image_size = extract<pointer_type>(user_data_ptr); // actually size_t
        const auto process_id = extract<std::uint32_t>(user_data_ptr);
        [[maybe_unused]]const auto ImageCheckSum = extract<std::uint32_t>(user_data_ptr);
        const auto TimeDateStamp = extract<std::uint32_t>(user_data_ptr);
        [[maybe_unused]]const auto Reserved0 = extract<std::uint32_t>(user_data_ptr);
        [[maybe_unused]]const auto DefaultBase = extract<pointer_type>(user_data_ptr);
        [[maybe_unused]]const auto Reserved1 = extract<std::uint32_t>(user_data_ptr);
        [[maybe_unused]]const auto Reserved2 = extract<std::uint32_t>(user_data_ptr);
        [[maybe_unused]]const auto Reserved3 = extract<std::uint32_t>(user_data_ptr);
        [[maybe_unused]]const auto Reserved4 = extract<std::uint32_t>(user_data_ptr);
        new_info.file_name = extract_wstring(user_data_ptr);

        if(process_id == process_of_interest)
        {
            const auto loaded_iter = std::ranges::find(loaded_modules, new_info);

            if(event_record->EventHeader.EventDescriptor.Opcode == 10 ||
                event_record->EventHeader.EventDescriptor.Opcode == 3)
            {
                if(loaded_iter == loaded_modules.end())
                {
                    new_info.active = true;
                    loaded_modules.push_back(new_info);
                }
                else
                {
                    loaded_iter->active = true;
                }
            }
            else if(event_record->EventHeader.EventDescriptor.Opcode == 2 ||
                event_record->EventHeader.EventDescriptor.Opcode == 4)
            {
                if(loaded_iter != loaded_modules.end())
                {
                    loaded_iter->active = false;
                    // loaded_modules.erase(loaded_iter);
                }
            }
        }
        
        assert((user_data_ptr - user_data_start) <= event_record->UserDataLength);
    }
    if(provider_guid == guid_perf_info && event_record->EventHeader.EventDescriptor.Opcode == 46)
    {
        profile_sample_data sample_data;

        sample_data.instruction_pointer = extract<pointer_type>(user_data_ptr);
        sample_data.thread_id = extract<std::uint32_t>(user_data_ptr);
        [[maybe_unused]] const auto count = extract<std::uint32_t>(user_data_ptr);
        
        assert((user_data_ptr - user_data_start) == event_record->UserDataLength);

        const auto thread_iter = thread_to_process.find(sample_data.thread_id);
        if(thread_iter != thread_to_process.end())
        {
            const auto proc_iter = known_processes.find(thread_iter->second);
            if(thread_iter->second == process_of_interest) //proc_iter != known_processes.end() && proc_iter->second  == "python.exe")
            {
                profile_samples[event_record->EventHeader.TimeStamp.QuadPart].push_back(sample_data);

                // const auto address = sample_data.instruction_pointer;

                // std::size_t in_modules = 0;
                // for(auto& mod : loaded_modules)
                // {
                //     // assert(mod.active);
                //     if(address >= mod.image_base && address < mod.image_base + mod.image_size)
                //     {
                //         ++mod.sample_self_count;
                //         ++in_modules;

                //         ++mod.per_address_count[address].sample_self_count;
                //     }
                // }
                // assert(in_modules <= 1);
            }
        }
    }
    if(provider_guid == guid_stack_walk && event_record->EventHeader.EventDescriptor.Opcode == 32)
    {
        const auto event_time_stamp = extract<std::uint64_t>(user_data_ptr);
        const auto process_id = extract<std::uint32_t>(user_data_ptr);
        const auto thread_id = extract<std::uint32_t>(user_data_ptr);

        const auto offset = user_data_ptr - user_data_start;
        const auto stack_size = (event_record->UserDataLength - offset) / sizeof(pointer_type);

        if(process_id == process_of_interest)
        {
            stack_trace stack;

            stack.frames.reserve(stack_size);
            bool has_debug_address = false;
            for(int i = 0; i < stack_size; ++i)
            {
                stack.frames.push_back(stack_frame{extract<pointer_type>(user_data_ptr)});
            }

            if(profile_samples.find(event_time_stamp) != profile_samples.end())
            {

            auto& sample_events = profile_samples.at(event_time_stamp);

            profile_sample_data* profile_sample = nullptr;
            for(auto& sample_event : sample_events)
            {
                if(sample_event.thread_id == thread_id)
                {
                    assert(profile_sample == nullptr);
                    profile_sample = &sample_event;
                }
            }
            assert(profile_sample != nullptr);

            profile_sample->stacks.push_back(std::move(stack));

            // proc_stacks_total++;

            // bool all_kernel_stack = true;
            // for(const auto frame : stack.frames)
            // {
            //     std::size_t in_modules = 0;
            //     for(auto& mod : loaded_modules)
            //     {
            //         if(frame.address >= mod.image_base && frame.address < mod.image_base + mod.image_size)
            //         {
            //             ++mod.stack_total_count;
            //             ++mod.per_address_count[frame.address].stack_total_count;
            //             ++in_modules;
            //         }
            //     }

            //     assert(in_modules <= 1);
                
            //     if(is_kernel_address(frame.address))
            //     {
            //         assert(in_modules == 0);
            //         ++kernel_samples;
            //         ++loaded_modules.front().stack_total_count;
            //         ++loaded_modules.front().per_address_count[frame.address].stack_total_count;
            //     }
            //     else
            //     {
            //         all_kernel_stack = false;
            //     }
            // }
        
            // {
            //     const auto top_frame = stack.frames.front();
            //     std::size_t in_modules = 0;
            //     for(auto& mod : loaded_modules)
            //     {
            //         if(top_frame.address >= mod.image_base && top_frame.address < mod.image_base + mod.image_size)
            //         {
            //             ++mod.stack_self_count;
            //             ++mod.per_address_count[top_frame.address].stack_self_count;
            //             ++in_modules;
            //         }
            //     }

            //     assert(in_modules <= 1);
                
            //     if(is_kernel_address(top_frame.address))
            //     {
            //         assert(in_modules == 0);
            //         ++kernel_samples;
            //         ++loaded_modules.front().stack_self_count;
            //         ++loaded_modules.front().per_address_count[top_frame.address].stack_self_count;
            //     }
            // }

            // if(all_kernel_stack)
            // {
            //     ++all_kernel_stacks;
            // }
            // else
            // {
            //     ++non_kernel_stacks;
            // }
            assert((user_data_ptr - user_data_start) == event_record->UserDataLength);
            }
        }
    }
}

module_info* find_module_for_address(std::uint64_t address)
{
    for(auto& mod : loaded_modules)
    {
        // assert(mod.active);
        if(address >= mod.image_base && address < mod.image_base + mod.image_size)
        {
            return &mod;
        }
    }
    return nullptr;
}


class CCallback : public IDiaLoadCallback2{
    int m_nRefCount;
public:
    CCallback() { m_nRefCount = 0; }

    //IUnknown
    ULONG STDMETHODCALLTYPE AddRef() {
        m_nRefCount++;
        return m_nRefCount;
    }
    ULONG STDMETHODCALLTYPE Release() {
        if ( (--m_nRefCount) == 0 )
            delete this;
        return m_nRefCount;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID rid, void **ppUnk ) {
        if ( ppUnk == NULL ) {
            return E_INVALIDARG;
        }
        if (rid == __uuidof( IDiaLoadCallback2 ) )
            *ppUnk = (IDiaLoadCallback2 *)this;
        else if (rid == __uuidof( IDiaLoadCallback ) )
            *ppUnk = (IDiaLoadCallback *)this;
        else if (rid == __uuidof( IUnknown ) )
            *ppUnk = (IUnknown *)this;
        else
            *ppUnk = NULL;
        if ( *ppUnk != NULL ) {
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE NotifyDebugDir(
                BOOL fExecutable, 
                DWORD cbData,
                BYTE data[]) // really a const struct _IMAGE_DEBUG_DIRECTORY *
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE NotifyOpenDBG(
                LPCOLESTR dbgPath, 
                HRESULT resultCode)
    {
        // wprintf(L"opening %s...\n", dbgPath);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE NotifyOpenPDB(
                LPCOLESTR pdbPath, 
                HRESULT resultCode)
    {
        // wprintf(L"opening %s...\n", pdbPath);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictRegistryAccess()         
    {
        // return hr != S_OK to prevent querying the registry for symbol search paths
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictSymbolServerAccess()
    {
      // return hr != S_OK to prevent accessing a symbol server
      return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictOriginalPathAccess()     
    {
        // return hr != S_OK to prevent querying the registry for symbol search paths
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictReferencePathAccess()
    {
        // return hr != S_OK to prevent accessing a symbol server
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictDBGAccess()
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictSystemRootAccess()
    {
        return S_OK;
    }
};

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}
bool replace(std::wstring& str, const std::wstring& from, const std::wstring& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void resolve_module_names()
{
    HRESULT hr = CoInitialize(NULL);
    for(auto& mod : loaded_modules)
    {
        auto path = mod.file_name;
        replace(path, L"\\Device\\HarddiskVolume2", L"C:");

        CComPtr<IDiaDataSource> dia_source;

        hr = CoCreateInstance(__uuidof(DiaSource),
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                __uuidof(IDiaDataSource),
                                (void **)&dia_source);

        CCallback callback; // Receives callbacks from the DIA symbol locating procedure,
                            // thus enabling a user interface to report on the progress of
                            // the location attempt. The client application may optionally
                            // provide a reference to its own implementation of this
                            // virtual base class to the IDiaDataSource::loadDataForExe method.
        callback.AddRef();

        hr = dia_source->loadDataForExe(path.c_str(), L"SRV**\\\\symbols\\symbols", &callback);

        if(hr != S_OK)
            continue;

        IDiaSession* session;
        if(FAILED(dia_source->openSession(&session)))
            continue;

        for(auto& [address, counts] : mod.per_address_count)
        {
            const auto rva = address - mod.image_base;
                    
            IDiaSymbol *symbol;
            hr = session->findSymbolByRVA(rva,SymTagEnum::SymTagFunction,&symbol);
            if(hr == S_OK)
            {
                BSTR name;
                symbol->get_name(&name);
                counts.function_name = name;
                SysFreeString(name);
                
                ULONGLONG length = 0;
                if(symbol->get_length(&length) == S_OK)
                {
                    IDiaEnumLineNumbers *lineNums[100];
                    if(session->findLinesByRVA(rva,length,lineNums) == S_OK)
                    {
                        auto &l = lineNums[0];
                        CComPtr<IDiaLineNumber> line;
                        IDiaLineNumber *lineNum;
                        ULONG fetched = 0;
                        for(uint8_t i=0;i<2;++i)
                        {
                            if(l->Next(i,&lineNum,&fetched) == S_OK && fetched == 1)
                            {
                                DWORD l;
                                IDiaSourceFile *srcFile;
                                if(lineNum->get_sourceFile(&srcFile) == S_OK)
                                {
                                    BSTR fileName;
                                    srcFile->get_fileName(&fileName);
                                    counts.file_name = fileName;
                                }
                                if(lineNum->get_lineNumber(&l) == S_OK)
                                {
                                    counts.line_number = l;
                                }
                            }
                        }
                    }
                }
            }
            
        }
    }
}

etl_reader::etl_reader(const std::filesystem::path& etl_file_path)
{
    EVENT_TRACE_LOGFILEW log_file;
    memset(&log_file, 0, sizeof(EVENT_TRACE_LOGFILEW));

    auto path = etl_file_path.native();

    log_file.LogFileName = path.data();
    log_file.LogFileMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    log_file.BufferCallback = nullptr;
    log_file.EventRecordCallback = &event_callback;

    auto handle = OpenTraceW(&log_file);
    if(handle == INVALID_PROCESSTRACE_HANDLE)
    {
        throw std::runtime_error("Could not open trace file");
    }

    proc_samples = 0;
    proc_stacks_total = 0;
    proc_stacks_self = 0;

    all_kernel_stacks = 0;
    non_kernel_stacks = 0;

    mod_samples = 0;
    mod_stacks_total = 0;
    mod_stacks_self = 0;

    kernel_samples = 0;

    debug_count = 0;
    debug_count_2 = 0;

    module_info kernel_module;
    kernel_module.file_name = L"KERNEL";
    kernel_module.image_base = 0;
    kernel_module.image_size = 0;

    loaded_modules.push_back(kernel_module);

    const auto result = ProcessTrace(&handle, 1, nullptr, nullptr);
    if(result != ERROR_SUCCESS)
    {
        throw std::runtime_error("Error while processing trace file");
    }

    for(const auto& [time, samples] : profile_samples)
    {
        assert(samples.size() == 1);
        const auto& sample = samples.front();
        auto* instruction_module = find_module_for_address(sample.instruction_pointer);

        const auto is_kernel_sample = is_kernel_address(sample.instruction_pointer);

        if(is_kernel_sample)
        {
            ++all_kernel_stacks;
        }
        else
        {
            if(instruction_module != nullptr)
            {
            ++instruction_module->sample_self_count;
            }
        }

        // assert(sample.stacks.size() > 0);
        assert(sample.stacks.size() <= 2);
        std::size_t has_user_stack = false;
        for(const auto& stack_trace : sample.stacks)
        {
            bool has_kernel_address = false;
            bool has_user_address = false;
            for(const auto& frame : stack_trace.frames)
            {
                if(is_kernel_address(frame.address))
                {
                    has_kernel_address = true;
                }
                else
                {
                    has_user_address = true;
                }
            }

            if(!has_user_address) continue;
            // if(has_kernel_address) continue;

            has_user_stack = true;

            bool is_top_frame = true;
            std::set<module_info*> modules;
            for(const auto& frame : stack_trace.frames)
            {
                if(is_kernel_address(frame.address))
                {
                    has_kernel_address = true;
                    auto* stack_module = &loaded_modules.front();
                    assert(!is_top_frame);
                    if(is_top_frame)
                    {
                        ++stack_module->stack_self_count;
                        ++stack_module->per_address_count[frame.address].stack_self_count;
                    }
                    else
                    {
                        ++stack_module->stack_total_count;
                        ++stack_module->per_address_count[frame.address].stack_total_count;
                    }
                }
                else
                {
                    auto* stack_module = find_module_for_address(frame.address);
                    assert(stack_module != nullptr);
                    modules.insert(stack_module);
                    if(is_top_frame)
                    {
                        assert(is_kernel_sample || stack_module == instruction_module);
                        ++stack_module->stack_self_count;
                        ++stack_module->per_address_count[frame.address].stack_self_count;
                    }
                    else
                    {
                        // ++stack_module->stack_total_count;
                        ++stack_module->per_address_count[frame.address].stack_total_count;
                    }
                    has_user_address = true;
                }

                is_top_frame = false;
            }
            for(auto mod : modules)
            {
                ++mod->stack_total_count;
            }
            assert(has_kernel_address || has_user_address);

            if(has_user_address)
            {
                has_user_stack = true;
            }

            break;
        }

        if(!has_user_stack)
        {
            int stop = 0;
            ++non_kernel_stacks;
        }
    }

    resolve_module_names();

    std::cout << "TOTAL PROC STACKS:  " << proc_stacks_total << "\n";
    std::cout << "KERNEL PROC STACKS:  " << all_kernel_stacks << "\n";
    std::cout << "NO KERNEL PROC STACKS:  " << non_kernel_stacks << "\n";
    std::cout << "TOTAL PROC SELF STACKS:  " << proc_stacks_self << "\n";
    std::cout << "TOTAL PROC SAMPLES: " << profile_samples.size() << "\n";
    std::size_t stack_total_sum = 0;
    std::size_t stack_self_sum = 0;
    std::size_t sample_self_sum = 0;
    for(const auto& mod : loaded_modules)
    {
        if(mod.stack_total_count == 0 && mod.stack_self_count == 0 && mod.sample_self_count == 0) continue;
        std::cout << "  MODULE " << std::filesystem::path(mod.file_name).filename() << ": "
                  << "STACK TOTAL " << mod.stack_total_count << " "
                  << "STACK SELF " << mod.stack_self_count << " "
                  << "SAMPLE SELF " << mod.sample_self_count << "\n";
    }
    std::cout << std::endl;
    for(const auto& mod : loaded_modules)
    {
        std::cout << "  MODULE " << std::filesystem::path(mod.file_name).filename() << ": "
                  << "STACK TOTAL " << mod.stack_total_count << " "
                  << "STACK SELF " << mod.stack_self_count << " "
                  << "SAMPLE SELF " << mod.sample_self_count << "\n";

        for(const auto& [address, counts] : mod.per_address_count)
        {
        std::wcout << "     " << counts.function_name;
        std::cout << " (" << std::format("{:#018x}", address) << "): "
                  << "STACK TOTAL " << counts.stack_total_count << " "
                  << "STACK SELF " << counts.stack_self_count << " "
                  << "SAMPLE SELF " << counts.sample_self_count << "\n";
        }

        stack_total_sum += mod.stack_total_count;
        stack_self_sum += mod.stack_self_count;
        sample_self_sum += mod.sample_self_count;
    }
    std::cout << "SUM STACK  TOTAL SAMPLES: " << stack_total_sum << "\n";
    std::cout << "SUM STACK  SELF  SAMPLES: " << stack_self_sum << "\n";
    std::cout << "SUM SAMPLES: " << sample_self_sum << "\n";
    std::cout << "KERNEL SAMPLES: " << kernel_samples << "\n";
    std::cout << "SUM ALL SAMPLES: " << sample_self_sum + kernel_samples << "\n";

    std::cout.flush();

    // for(const auto& [guid, count_py_opcode] : count_by_provider)
    // {
    //     print_guid(guid);
    //     std::cout << ":\n";
    //     for(const auto& [opcode, count] : count_py_opcode)
    //     {
    //         std::cout << "  " << int(opcode) << ": " << count << "\n";
    //     }
    // }

    CloseTrace(handle);
}