
#include <snail/perf_data/dispatching_event_observer.hpp>
#include <snail/perf_data/perf_data_file.hpp>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/parser/records/perf.hpp>

using namespace snail;

int main(int /*argc*/, char* /*argv[]*/)
{
    perf_data::perf_data_file file(R"(C:\Users\aziegenhagel\source\snail-support\tests\data\hidden_perf.data)");

    perf_data::dispatching_event_observer observer;

    std::size_t samples_since_last_round = 0;

    // observer.register_event<perf_data::parser::id_index_event_view>(
    //     [](const perf_data::parser::event_header_view& /*header*/,
    //        const perf_data::parser::id_index_event_view& event)
    //     {
    //         [[maybe_unused]] const auto nr = event.nr();
    //         for(std::uint64_t i = 0; i < nr; ++i)
    //         {
    //             const auto entry = event.entry(i);
    //             [[maybe_unused]] const auto cpu = entry.cpu();
    //             [[maybe_unused]] const auto tid = entry.tid();
    //             [[maybe_unused]] const auto id = entry.id();
    //             [[maybe_unused]] const auto idx = entry.idx();
    //             [[maybe_unused]] int stop = 0;
    //         }
    //     });
    // observer.register_event<perf_data::parser::thread_map_event_view>(
    //     [](const perf_data::parser::event_header_view& /*header*/,
    //        const perf_data::parser::thread_map_event_view& event)
    //     {
    //         [[maybe_unused]] const auto nr = event.nr();
    //         for(std::uint64_t i = 0; i < nr; ++i)
    //         {
    //             const auto entry = event.entry(i);
    //             [[maybe_unused]] const auto pid = entry.pid();
    //             [[maybe_unused]] const auto comm = entry.comm();
    //             [[maybe_unused]] int stop = 0;
    //         }
    //     });
    // observer.register_event<perf_data::parser::cpu_map_event_view>(
    //     [](const perf_data::parser::event_header_view& /*header*/,
    //        const perf_data::parser::cpu_map_event_view& event)
    //     {
    //         switch(event.type())
    //         {
    //         case perf_data::parser::cpu_map_type::cpus:
    //         {
    //             [[maybe_unused]] const auto data  = event.cpus_data();
    //             [[maybe_unused]] const auto count = data.nr();
    //             break;
    //         }
    //         case perf_data::parser::cpu_map_type::mask:
    //         {
    //             [[maybe_unused]] const auto data  = event.mask_data();
    //             [[maybe_unused]] const auto count = data.nr();
    //             [[maybe_unused]] const auto long_size = data.long_size();
    //             for(std::uint64_t i = 0; i < count; ++i)
    //             {
    //                 [[maybe_unused]] const auto mask = data.mask_32(i);
    //                 [[maybe_unused]] int stop = 0;
    //             }
    //             break;
    //         }
    //         case perf_data::parser::cpu_map_type::range_cpus:
    //         {
    //             [[maybe_unused]] const auto data      = event.range_cpus_data();
    //             [[maybe_unused]] const auto any_cpu   = data.any_cpu();
    //             [[maybe_unused]] const auto start_cpu = data.start_cpu();
    //             [[maybe_unused]] const auto end_cpu   = data.end_cpu();
    //             break;
    //         }
    //         }
    //     });
    observer.register_event<perf_data::parser::comm_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::comm_event_view& event)
        {
            std::cout << std::format("COMM @{} ({},{}): {}", *event.sample_id().time, event.pid(), event.tid(), event.comm()) << "\n";
        });
    observer.register_event<perf_data::parser::mmap2_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::mmap2_event_view& event)
        {
            std::cout << std::format("MMAP @{} ({},{}): {} @{} + {}", *event.sample_id().time, event.pid(), event.tid(), event.filename(), event.addr(), event.len()) << "\n";
        });
    observer.register_event<perf_data::parser::fork_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::fork_event_view& event)
        {
            std::cout << std::format("FORK @{}|{} ({},{}) -> ({},{})", *event.sample_id().time, event.time(), event.ppid(), event.ptid(), event.pid(), event.tid()) << "\n";
        });
    observer.register_event<perf_data::parser::exit_event_view>(
        [](const perf_data::parser::event_header_view& /*header*/,
           const perf_data::parser::exit_event_view& event)
        {
            std::cout << std::format("EXIT @{}|{} ({},{}) -> ({},{})", *event.sample_id().time, event.time(), event.ppid(), event.ptid(), event.pid(), event.tid()) << "\n";
        });
    observer.register_event<perf_data::parser::finished_round_event_view>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::finished_round_event_view& /*event*/)
        {
            std::cout << std::format("FINISHED ROUND: {} samples", samples_since_last_round) << "\n";
            samples_since_last_round = 0;
        });

    observer.register_event<perf_data::parser::sample_event>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::sample_event& event)
        {
            [[maybe_unused]] const auto ip = event.ip;
            ++samples_since_last_round;
        });

    file.process(observer);
}