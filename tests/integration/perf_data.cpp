
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

#include <folders.hpp>

#include <snail/perf_data/metadata.hpp>
#include <snail/perf_data/parser/event.hpp>
#include <snail/perf_data/perf_data_file.hpp>

using namespace snail;
using namespace snail::detail::tests;

using namespace std::string_literals;

namespace {

class counting_event_observer : public perf_data::event_observer
{
public:
    std::map<perf_data::parser::event_type, std::size_t> counts;

    void handle(const perf_data::parser::event_header_view& event_header, const perf_data::parser::event_attributes& /*attributes*/, std::span<const std::byte> /*event_data*/, std::endian /*byte_order*/) override
    {
        ++counts[event_header.type()];
    }
    void handle(const perf_data::parser::event_header_view& event_header, std::span<const std::byte> /*event_data*/, std::endian /*byte_order*/) override
    {
        ++counts[event_header.type()];
    }
};

} // namespace

TEST(PerfDataFile, ReadInner)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto file_path = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "linux" / "deb" / "record" / "inner-perf.data";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    perf_data::perf_data_file file(file_path);

    counting_event_observer counting_observer;

    file.process(counting_observer);

    EXPECT_EQ(file.metadata().hostname, "DESKTOP-0RH3TEF");
    EXPECT_EQ(file.metadata().os_release, "5.15.90.1-microsoft-standard-WSL2");
    EXPECT_EQ(file.metadata().version, "6.3.3");
    EXPECT_EQ(file.metadata().arch, "x86_64");
    EXPECT_EQ(file.metadata().nr_cpus->nr_cpus_available, 8);
    EXPECT_EQ(file.metadata().nr_cpus->nr_cpus_online, 8);
    EXPECT_EQ(file.metadata().cpu_desc, "Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz");
    EXPECT_EQ(file.metadata().cpu_id, "GenuineIntel,6,158,9");
    EXPECT_EQ(file.metadata().total_mem, 8058816);
    EXPECT_EQ(file.metadata().cmdline, (std::vector<std::string>{
                                           "/usr/bin/perf",
                                           "record",
                                           "-g",
                                           "-o",
                                           "inner-perf.data",
                                           "/tmp/build/inner/Debug/build/inner"}));
    EXPECT_EQ(file.metadata().event_desc.size(), 1);
    EXPECT_EQ(file.metadata().event_desc[0].attribute.type, perf_data::parser::attribute_type::hardware);
    EXPECT_EQ(file.metadata().event_desc[0].attribute.sample_period_freq, 4000);
    EXPECT_EQ(file.metadata().event_desc[0].attribute.sample_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000100100111"));
    EXPECT_EQ(file.metadata().event_desc[0].attribute.read_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000000000100"));
    EXPECT_EQ(file.metadata().event_desc[0].attribute.flags.data(), std::bitset<64>("0000000000000000000000000000000001100001100101000011011100100011"));
    EXPECT_EQ(file.metadata().event_desc[0].attribute.precise_ip, perf_data::parser::skid_constraint_type::can_have_arbitrary_skid);
    EXPECT_FALSE(file.metadata().event_desc[0].attribute.name.has_value());
    EXPECT_EQ(file.metadata().event_desc[0].event_string, "cycles:u");
    EXPECT_EQ(file.metadata().event_desc[0].ids, (std::vector<std::size_t>{9, 10, 11, 12, 13, 14, 15, 16}));
    EXPECT_EQ(file.metadata().sample_time->start, std::chrono::nanoseconds(203870699200));
    EXPECT_EQ(file.metadata().sample_time->end, std::chrono::nanoseconds(204258097600));
    EXPECT_FALSE(file.metadata().clockid.has_value());

    const std::map<perf_data::parser::event_type, std::size_t> expected_event_counts = {
        {perf_data::parser::event_type::comm,           2   },
        {perf_data::parser::event_type::exit,           1   },
        {perf_data::parser::event_type::sample,         1524},
        {perf_data::parser::event_type::mmap2,          7   },
        {perf_data::parser::event_type::finished_round, 1   },
        {perf_data::parser::event_type::id_index,       1   },
        {perf_data::parser::event_type::thread_map,     1   },
        {perf_data::parser::event_type::cpu_map,        1   },
        {perf_data::parser::event_type::finished_init,  1   }
    };

    EXPECT_EQ(counting_observer.counts, expected_event_counts);
}
