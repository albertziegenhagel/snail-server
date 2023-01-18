
#include <gtest/gtest.h>

#include <snail/perf_data/dispatching_event_observer.hpp>
#include <snail/perf_data/perf_data_file.hpp>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/parser/records/perf.hpp>

using namespace snail;

TEST(PerfDataFile, Process)
{
    perf_data::perf_data_file file(R"(C:\Users\aziegenhagel\source\snail-support\tests\data\hidden_perf.data)");

    perf_data::dispatching_event_observer observer;

    observer.register_event<perf_data::parser::id_index_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::id_index_event_view& event)
        {
            [[maybe_unused]] const auto nr = event.nr();
        });
    observer.register_event<perf_data::parser::thread_map_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::thread_map_event_view& event)
        {
            [[maybe_unused]] const auto nr = event.nr();
        });
    observer.register_event<perf_data::parser::cpu_map_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::cpu_map_event_view& event)
        {
            switch(event.type())
            {
            case perf_data::parser::cpu_map_type::cpus:
            {
                [[maybe_unused]] const auto data  = event.cpus_data();
                [[maybe_unused]] const auto count = data.nr();
                break;
            }
            case perf_data::parser::cpu_map_type::mask:
            {
                [[maybe_unused]] const auto data  = event.mask_data();
                [[maybe_unused]] const auto count = data.nr();
                break;
            }
            case perf_data::parser::cpu_map_type::range_cpus:
            {
                [[maybe_unused]] const auto data      = event.range_cpus_data();
                [[maybe_unused]] const auto any_cpu   = data.any_cpu();
                [[maybe_unused]] const auto start_cpu = data.start_cpu();
                [[maybe_unused]] const auto end_cpu   = data.end_cpu();
                break;
            }
            }
        });
    observer.register_event<perf_data::parser::comm_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::comm_event_view& event)
        {
            [[maybe_unused]] const auto pid = event.pid();
        });
    observer.register_event<perf_data::parser::mmap2_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::mmap2_event_view& event)
        {
            [[maybe_unused]] const auto pid = event.pid();
        });
    observer.register_event<perf_data::parser::fork_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::fork_event_view& event)
        {
            [[maybe_unused]] const auto pid = event.pid();
        });
    observer.register_event<perf_data::parser::exit_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::exit_event_view& event)
        {
            [[maybe_unused]] const auto pid = event.pid();
        });

    observer.register_event<perf_data::parser::sample_event>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::sample_event& event)
        {
            [[maybe_unused]] const auto ip = event.ip;
        });

    file.process(observer);
}