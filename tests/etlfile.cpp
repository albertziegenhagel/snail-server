
#include <map>

#include <gtest/gtest.h>

#include <etl/etlfile.hpp>
#include <etl/parser/trace_headers/system_trace.hpp>
#include <etl/parser/trace_headers/perfinfo_trace.hpp>

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
}

class my_observer : public etl::event_observer
{
public:
    my_observer()
    {

    }

    virtual void handle(const etl::parser::system_trace_header& header, std::span<std::byte> user_data) override
    {
        ++counters[header.packet.group][header.packet.type];
    }
    virtual void handle(const etl::parser::perfinfo_trace_header& header, std::span<std::byte> user_data) override
    {
        ++counters[header.packet.group][header.packet.type];
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