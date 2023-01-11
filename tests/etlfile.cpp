
#include <map>

#include <gtest/gtest.h>

#include <etl/etlfile.hpp>

#include <etl/parser/trace_headers/system_trace.hpp>
#include <etl/parser/trace_headers/perfinfo_trace.hpp>

#include <etl/parser/records/kernel/process.hpp>
#include <etl/parser/records/kernel/thread.hpp>
#include <etl/parser/records/kernel/image.hpp>
#include <etl/parser/records/kernel/perfinfo.hpp>
#include <etl/parser/records/kernel/stackwalk.hpp>

using namespace perfreader;

const char* group_to_string(etl::parser::event_trace_group group)
{
    switch(group)
    {
        case etl::parser::event_trace_group::header: return "header";
        case etl::parser::event_trace_group::io: return "io";
        case etl::parser::event_trace_group::memory: return "memory";
        case etl::parser::event_trace_group::process: return "process";
        case etl::parser::event_trace_group::file: return "file";
        case etl::parser::event_trace_group::thread: return "thread";
        case etl::parser::event_trace_group::tcpip: return "tcpip";
        case etl::parser::event_trace_group::job: return "job";
        case etl::parser::event_trace_group::udpip: return "udpip";
        case etl::parser::event_trace_group::registry: return "registry";
        case etl::parser::event_trace_group::dbgprint: return "dbgprint";
        case etl::parser::event_trace_group::config: return "config";
        case etl::parser::event_trace_group::spare1: return "spare1";
        case etl::parser::event_trace_group::wnf: return "wnf";
        case etl::parser::event_trace_group::pool: return "pool";
        case etl::parser::event_trace_group::perfinfo: return "perfinfo";
        case etl::parser::event_trace_group::heap: return "heap";
        case etl::parser::event_trace_group::object: return "object";
        case etl::parser::event_trace_group::power: return "power";
        case etl::parser::event_trace_group::modbound: return "modbound";
        case etl::parser::event_trace_group::image: return "image";
        case etl::parser::event_trace_group::dpc: return "dpc";
        case etl::parser::event_trace_group::cc: return "cc";
        case etl::parser::event_trace_group::critsec: return "critsec";
        case etl::parser::event_trace_group::stackwalk: return "stackwalk";
        case etl::parser::event_trace_group::ums: return "ums";
        case etl::parser::event_trace_group::alpc: return "alpc";
        case etl::parser::event_trace_group::splitio: return "splitio";
        case etl::parser::event_trace_group::thread_pool: return "thread_pool";
        case etl::parser::event_trace_group::hypervisor: return "hypervisor";
        case etl::parser::event_trace_group::hypervisorx: return "hypervisorx";
    }
    return "UNKNOWN";
}

template<typename T, std::size_t N>
bool contains(const std::array<T, N>& data, const T& entry)
{
    return std::ranges::find(data, entry) != std::ranges::end(data);
}

class my_observer : public etl::event_observer
{
public:
    my_observer()
    {

    }

    virtual void handle(const etl::etl_file::header_data& file_header,
                        const etl::parser::system_trace_header_view& trace_header,
                        std::span<const std::byte> user_data) override
    {
        ++counters[trace_header.packet().group()][trace_header.packet().type()];

        if((trace_header.packet().group() == etl::parser::event_trace_group::image &&
           contains(etl::parser::image_v2_load_event_view::event_types, trace_header.packet().type()) &&
           contains(etl::parser::image_v2_load_event_view::event_versions, trace_header.version())) ||
           // for some unknown reason, the LOAD event is reported in the process group
           (trace_header.packet().group() == etl::parser::event_trace_group::process &&
           trace_header.packet().type() == static_cast<std::uint8_t>(etl::parser::image_v2_load_event_view::event_type::load) &&
           contains(etl::parser::image_v2_load_event_view::event_versions, trace_header.version())))
        {
            [[maybe_unused]] auto event = etl::parser::image_v2_load_event_view(user_data, file_header.pointer_size);
        }
        else if((trace_header.packet().group() == etl::parser::event_trace_group::process &&
           contains(etl::parser::process_v4_type_group1_event_view::event_types, trace_header.packet().type()) &&
           contains(etl::parser::process_v4_type_group1_event_view::event_versions, trace_header.version())))
        {
            [[maybe_unused]] auto event = etl::parser::process_v4_type_group1_event_view(user_data, file_header.pointer_size);
        }
        else if((trace_header.packet().group() == etl::parser::event_trace_group::thread &&
           contains(etl::parser::thread_v3_type_group1_event_view::event_types, trace_header.packet().type()) &&
           contains(etl::parser::thread_v3_type_group1_event_view::event_versions, trace_header.version())))
        {
            [[maybe_unused]] auto event = etl::parser::thread_v3_type_group1_event_view(user_data, file_header.pointer_size);
        }
        else if((trace_header.packet().group() == etl::parser::event_trace_group::thread &&
           contains(etl::parser::thread_v4_type_group1_event_view::event_types, trace_header.packet().type()) &&
           contains(etl::parser::thread_v4_type_group1_event_view::event_versions, trace_header.version())))
        {
            [[maybe_unused]] auto event = etl::parser::thread_v4_type_group1_event_view(user_data, file_header.pointer_size);
        }
    }
    virtual void handle(const etl::etl_file::header_data& file_header,
                        const etl::parser::perfinfo_trace_header_view& trace_header,
                        std::span<const std::byte> user_data) override
    {
        ++counters[trace_header.packet().group()][trace_header.packet().type()];

        if((trace_header.packet().group() == etl::parser::event_trace_group::perfinfo &&
           contains(etl::parser::perfinfo_v2_sampled_profile_event_view::event_types, trace_header.packet().type()) &&
           contains(etl::parser::perfinfo_v2_sampled_profile_event_view::event_versions, trace_header.version())))
        {
            [[maybe_unused]] auto event = etl::parser::perfinfo_v2_sampled_profile_event_view(user_data, file_header.pointer_size);
        }
        else if((trace_header.packet().group() == etl::parser::event_trace_group::stackwalk &&
           contains(etl::parser::stackwalk_v2_stack_event_view::event_types, trace_header.packet().type()) &&
           contains(etl::parser::stackwalk_v2_stack_event_view::event_versions, trace_header.version())))
        {
            [[maybe_unused]] auto event = etl::parser::stackwalk_v2_stack_event_view(user_data, file_header.pointer_size);
        }
    }

    void finish()
    {
        for(const auto& [group, counts_per_type] : counters)
        {
            std::size_t sum = 0;
            for(const auto& [type, count] : counts_per_type)
            {
                sum += count;
            }
            std::cout << "GROUP " << group_to_string(group) << ": " << sum << "\n";
            for(const auto& [type, count] : counts_per_type)
            {
                std::cout << "  TYPE " << (int)type << ": " << count << "\n";
            }
        }
        std::cout.flush();
    }

private:
    std::map<etl::parser::event_trace_group, std::map<std::uint8_t, std::size_t>> counters;
};

TEST(EtlFile, Process)
{
    etl::etl_file file(R"(C:\Users\aziegenhagel\source\perfreader\tests\data\sc.user_aux.etl)");

    my_observer observer;
    file.process(observer);

    observer.finish();
}