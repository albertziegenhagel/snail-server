

#include <gtest/gtest.h>

#include <etl/etlfile.hpp>

#include <etl/dispatching_event_observer.hpp>

#include <etl/parser/records/kernel/process.hpp>
#include <etl/parser/records/kernel/thread.hpp>
#include <etl/parser/records/kernel/image.hpp>
#include <etl/parser/records/kernel/perfinfo.hpp>
#include <etl/parser/records/kernel/stackwalk.hpp>

#include <etl/parser/records/kernel_trace_control/image_id.hpp>

#include <etl/detail/dump.hpp>

using namespace snail;

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

TEST(EtlFile, Process)
{
    etl::etl_file file(R"(C:\Users\aziegenhagel\source\snail-support\tests\data\hidden_sc.user_aux.etl)");

    etl::dispatching_event_observer observer;

    observer.register_event<etl::parser::stackwalk_v2_stack_event_view>(
        [](const etl::common_trace_header& /*header*/,
           const etl::parser::stackwalk_v2_stack_event_view& event)
        {
            [[maybe_unused]] const auto stack_size = event.stack_size();
        });
    observer.register_event<etl::parser::perfinfo_v2_sampled_profile_event_view>(
        [](const etl::common_trace_header& /*header*/,
           const etl::parser::perfinfo_v2_sampled_profile_event_view& event)
        {
            [[maybe_unused]] const auto instruction_pointer = event.instruction_pointer();
            
            // std::visit([]<typename T>(const T& header) {
            //     etl::detail::dump_buffer(header.buffer(), 0, header.packet().size());
            // }, header);
            // etl::detail::dump_buffer(event.buffer(), 0, event.buffer().size());
            // exit(0);
        });
    observer.register_event<etl::parser::process_v4_type_group1_event_view>(
        [](const etl::common_trace_header& /*header*/,
           const etl::parser::process_v4_type_group1_event_view& event)
        {
            [[maybe_unused]] const auto process_id = event.process_id();
        });
    observer.register_event<etl::parser::thread_v3_type_group1_event_view>(
        [](const etl::common_trace_header& /*header*/,
           const etl::parser::thread_v3_type_group1_event_view& event)
        {
            [[maybe_unused]] const auto process_id = event.process_id();
        });
    observer.register_event<etl::parser::image_v2_load_event_view>(
        [](const etl::common_trace_header& /*header*/,
           const etl::parser::image_v2_load_event_view& event)
        {
            [[maybe_unused]] const auto image_base = event.image_base();
        });
        
    observer.register_event<etl::parser::image_id_v2_info_event_view>(
        [](const etl::common_trace_header& /*header*/,
           const etl::parser::image_id_v2_info_event_view& event)
        {
            [[maybe_unused]] const auto image_base = event.image_base();
        });

    file.process(observer);

    // observer.finish();
}