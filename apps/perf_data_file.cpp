
#include <format>

#include <snail/perf_data/dispatching_event_observer.hpp>
#include <snail/perf_data/perf_data_file.hpp>

#include <snail/perf_data/parser/records/kernel.hpp>
#include <snail/perf_data/parser/records/perf.hpp>

using namespace snail;

int main(int /*argc*/, char* /*argv*/[])
{
    perf_data::perf_data_file file(R"(C:\Users\aziegenhagel\source\snail-support\tests\data\hidden_debug_perf.data)");

    perf_data::dispatching_event_observer observer;

    struct
    {
        std::size_t count = 0;

        std::uint64_t first_time = std::numeric_limits<std::uint64_t>::max();
        std::uint64_t last_time  = std::numeric_limits<std::uint64_t>::min();
    } samples_data;

    const auto flush_samples = [&samples_data]()
    {
        if(samples_data.count == 0) return;
        std::cout << std::format("SAMPLES @{}-{} count {}", samples_data.first_time, samples_data.last_time, samples_data.count) << "\n";

        samples_data.count      = 0;
        samples_data.first_time = std::numeric_limits<std::uint64_t>::max();
        samples_data.last_time  = std::numeric_limits<std::uint64_t>::min();
    };

    observer.register_event<perf_data::parser::id_index_event_view>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::id_index_event_view& event)
        {
            flush_samples();
            std::cout << std::format("ID INDEX; nr {}:", event.nr()) << "\n";
            const auto nr = event.nr();
            for(std::uint64_t i = 0; i < nr; ++i)
            {
                const auto entry = event.entry(i);
                std::cout << std::format("  cpu {} tid {}: id {} idx {}", entry.cpu(), entry.tid(), entry.id(), entry.idx()) << "\n";
            }
        });
    observer.register_event<perf_data::parser::thread_map_event_view>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::thread_map_event_view& event)
        {
            flush_samples();
            std::cout << std::format("THREAD MAP; nr {}:", event.nr()) << "\n";
            const auto nr = event.nr();
            for(std::uint64_t i = 0; i < nr; ++i)
            {
                const auto entry = event.entry(i);
                std::cout << std::format("  pid {} comm '{}'", entry.pid(), entry.comm()) << "\n";
            }
        });
    observer.register_event<perf_data::parser::cpu_map_event_view>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::cpu_map_event_view& event)
        {
            flush_samples();
            switch(event.type())
            {
            case perf_data::parser::cpu_map_type::cpus:
            {
                std::cout << std::format("CPU MAP (CPUS)") << "\n";
                [[maybe_unused]] const auto data  = event.cpus_data();
                [[maybe_unused]] const auto count = data.nr();
                break;
            }
            case perf_data::parser::cpu_map_type::mask:
            {
                const auto data      = event.mask_data();
                const auto nr        = data.nr();
                const auto long_size = data.long_size();
                std::cout << std::format("CPU MAP (MASK) nr {} long size {}", nr, long_size) << "\n";
                for(std::uint64_t i = 0; i < nr; ++i)
                {
                    std::cout << std::format("  mask {}", long_size == 4 ? data.mask_32(i) : data.mask_64(i)) << "\n";
                }
                break;
            }
            case perf_data::parser::cpu_map_type::range_cpus:
            {
                std::cout << std::format("CPU MAP (RANGE)") << "\n";
                [[maybe_unused]] const auto data      = event.range_cpus_data();
                [[maybe_unused]] const auto any_cpu   = data.any_cpu();
                [[maybe_unused]] const auto start_cpu = data.start_cpu();
                [[maybe_unused]] const auto end_cpu   = data.end_cpu();
                break;
            }
            }
        });
    observer.register_event<perf_data::parser::comm_event_view>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::comm_event_view& event)
        {
            flush_samples();
            std::cout << std::format("COMM @{} ({},{}): {}", *event.sample_id().time, event.pid(), event.tid(), event.comm()) << "\n";
        });
    observer.register_event<perf_data::parser::mmap2_event_view>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::mmap2_event_view& event)
        {
            flush_samples();
            std::cout << std::format("MMAP2 @{} ({},{}): {} @{:#018x} + {:#018x} ; {:#18x}", *event.sample_id().time, event.pid(), event.tid(), event.filename(), event.addr(), event.len(), event.pgoff()) << "\n";
        });
    observer.register_event<perf_data::parser::fork_event_view>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::fork_event_view& event)
        {
            flush_samples();
            std::cout << std::format("FORK @{}|{} ({},{}) -> ({},{})", *event.sample_id().time, event.time(), event.ppid(), event.ptid(), event.pid(), event.tid()) << "\n";
        });
    observer.register_event<perf_data::parser::exit_event_view>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::exit_event_view& event)
        {
            flush_samples();
            std::cout << std::format("EXIT @{}|{} ({},{}) -> ({},{})", *event.sample_id().time, event.time(), event.ppid(), event.ptid(), event.pid(), event.tid()) << "\n";
        });
    observer.register_event<perf_data::parser::finished_round_event_view>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::finished_round_event_view& /*event*/)
        {
            flush_samples();
            std::cout << std::format("FINISHED ROUND") << "\n";
        });
    observer.register_event<perf_data::parser::finished_init_event_view>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::finished_init_event_view& /*event*/)
        {
            flush_samples();
            std::cout << std::format("FINISHED INIT") << "\n";
        });

    observer.register_event<perf_data::parser::sample_event>(
        [&](const perf_data::parser::event_header_view& /*header*/,
            const perf_data::parser::sample_event& event)
        {
            ++samples_data.count;
            samples_data.first_time = std::min(samples_data.first_time, *event.time);
            samples_data.last_time  = std::max(samples_data.last_time, *event.time);
        });

    file.process(observer);
}