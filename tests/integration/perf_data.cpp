
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include <folders.hpp>

#include <snail/perf_data/metadata.hpp>
#include <snail/perf_data/parser/event.hpp>
#include <snail/perf_data/perf_data_file.hpp>

using namespace snail;
using namespace snail::detail::tests;

using namespace std::string_literals;
using namespace std::chrono_literals;

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

std::string_view event_type_to_string(perf_data::parser::event_type event_type)
{
    switch(event_type)
    {
    case perf_data::parser::event_type::mmap: return "mmap";
    case perf_data::parser::event_type::lost: return "lost";
    case perf_data::parser::event_type::comm: return "comm";
    case perf_data::parser::event_type::exit: return "exit";
    case perf_data::parser::event_type::throttle: return "throttle";
    case perf_data::parser::event_type::unthrottle: return "unthrottle";
    case perf_data::parser::event_type::fork: return "fork";
    case perf_data::parser::event_type::read: return "read";
    case perf_data::parser::event_type::sample: return "sample";
    case perf_data::parser::event_type::mmap2: return "mmap2";
    case perf_data::parser::event_type::aux: return "aux";
    case perf_data::parser::event_type::itrace_start: return "itrace_start";
    case perf_data::parser::event_type::lost_samples: return "lost_samples";
    case perf_data::parser::event_type::switch_: return "switch_";
    case perf_data::parser::event_type::switch_cpu_wide: return "switch_cpu_wide";
    case perf_data::parser::event_type::namespaces: return "namespaces";
    case perf_data::parser::event_type::ksymbol: return "ksymbol";
    case perf_data::parser::event_type::bpf_event: return "bpf_event";
    case perf_data::parser::event_type::cgroup: return "cgroup";
    case perf_data::parser::event_type::text_poke: return "text_poke";
    case perf_data::parser::event_type::aux_output_hw_id: return "aux_output_hw_id";
    case perf_data::parser::event_type::header_attr: return "header_attr";
    case perf_data::parser::event_type::header_event_type: return "header_event_type";
    case perf_data::parser::event_type::header_tracing_data: return "header_tracing_data";
    case perf_data::parser::event_type::header_build_id: return "header_build_id";
    case perf_data::parser::event_type::finished_round: return "finished_round";
    case perf_data::parser::event_type::id_index: return "id_index";
    case perf_data::parser::event_type::auxtrace_info: return "auxtrace_info";
    case perf_data::parser::event_type::auxtrace: return "auxtrace";
    case perf_data::parser::event_type::auxtrace_error: return "auxtrace_error";
    case perf_data::parser::event_type::thread_map: return "thread_map";
    case perf_data::parser::event_type::cpu_map: return "cpu_map";
    case perf_data::parser::event_type::stat_config: return "stat_config";
    case perf_data::parser::event_type::stat: return "stat";
    case perf_data::parser::event_type::stat_round: return "stat_round";
    case perf_data::parser::event_type::event_update: return "event_update";
    case perf_data::parser::event_type::time_conv: return "time_conv";
    case perf_data::parser::event_type::header_feature: return "header_feature";
    case perf_data::parser::event_type::compressed: return "compressed";
    case perf_data::parser::event_type::finished_init: return "finished_init";
    }
    return "UNKNOWN";
}

struct test_progress_listener : public common::progress_listener
{
    using common::progress_listener::progress_listener;

    virtual void start([[maybe_unused]] std::string_view                title,
                       [[maybe_unused]] std::optional<std::string_view> message) const override
    {
        started = true;
    }

    virtual void report(double                                           progress,
                        [[maybe_unused]] std::optional<std::string_view> message) const override
    {
        reported.push_back(std::round(progress * 1000.0) / 10.0); // percent rounded to one digit after the point
        if(cancel_at && progress >= *cancel_at)
        {
            token.cancel();
        }
    }

    virtual void finish([[maybe_unused]] std::optional<std::string_view> message) const override
    {
        finished = true;
    }

    mutable bool                       started  = false;
    mutable bool                       finished = false;
    mutable std::vector<double>        reported;
    mutable common::cancellation_token token;

    std::optional<double> cancel_at = std::nullopt;
};

} // namespace

namespace snail::perf_data::parser {

std::ostream& operator<<(std::ostream& stream, const perf_data::parser::event_type& event_type)
{
    stream << event_type_to_string(event_type);
    return stream;
}

} // namespace snail::perf_data::parser

TEST(PerfDataFile, ReadInner)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto file_path = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "linux" / "deb" / "record" / "inner-perf.data";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    perf_data::perf_data_file file(file_path);

    counting_event_observer counting_observer;

    const test_progress_listener progress_listener(0.1);

    file.process(counting_observer, &progress_listener);

    EXPECT_TRUE(progress_listener.started);
    EXPECT_TRUE(progress_listener.finished);
    EXPECT_EQ(progress_listener.reported,
              (std::vector<double>{0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}));

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
    EXPECT_EQ(file.metadata().sample_time->start, 203870699200ns);
    EXPECT_EQ(file.metadata().sample_time->end, 204258097600ns);
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

TEST(PerfDataFile, ReadOrdered)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto file_path = get_root_dir().value() / "tests" / "apps" / "ordered" / "dist" / "linux" / "deb" / "record" / "ordered-perf.data";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    perf_data::perf_data_file file(file_path);

    counting_event_observer counting_observer;

    file.process(counting_observer);

    EXPECT_EQ(file.metadata().hostname, "DESKTOP-0RH3TEF");
    EXPECT_EQ(file.metadata().os_release, "5.15.146.1-microsoft-standard-WSL2");
    EXPECT_EQ(file.metadata().version, "6.3.3");
    EXPECT_EQ(file.metadata().arch, "x86_64");
    EXPECT_EQ(file.metadata().nr_cpus->nr_cpus_available, 8);
    EXPECT_EQ(file.metadata().nr_cpus->nr_cpus_online, 8);
    EXPECT_EQ(file.metadata().cpu_desc, "Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz");
    EXPECT_EQ(file.metadata().cpu_id, "GenuineIntel,6,158,9");
    EXPECT_EQ(file.metadata().total_mem, 8058956);
    EXPECT_EQ(*file.metadata().cmdline, (std::vector<std::string>{
                                            "/usr/bin/perf",
                                            "record",
                                            "-g",
                                            "-e", "cycles,branch-misses,cache-references,cache-misses",
                                            "-F", "5000",
                                            "-o",
                                            "ordered-perf.data",
                                            "/tmp/build/ordered/Debug/build/ordered"}));
    EXPECT_EQ(file.metadata().event_desc.size(), 4);
    EXPECT_EQ(file.metadata().event_desc[0].attribute.type, perf_data::parser::attribute_type::hardware);
    EXPECT_EQ(file.metadata().event_desc[0].attribute.sample_period_freq, 5000);
    EXPECT_EQ(file.metadata().event_desc[0].attribute.sample_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000101100111"));
    EXPECT_EQ(file.metadata().event_desc[0].attribute.read_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000000010100"));
    EXPECT_EQ(file.metadata().event_desc[0].attribute.flags.data(), std::bitset<64>("0000000000000000000000000000000001100001100101000011011101100011"));
    EXPECT_EQ(file.metadata().event_desc[0].attribute.precise_ip, perf_data::parser::skid_constraint_type::can_have_arbitrary_skid);
    EXPECT_FALSE(file.metadata().event_desc[0].attribute.name.has_value());
    EXPECT_EQ(file.metadata().event_desc[0].event_string, "cycles:u");
    EXPECT_EQ(file.metadata().event_desc[0].ids, (std::vector<std::size_t>{38, 39, 40, 41, 42, 43, 44, 45}));

    EXPECT_EQ(file.metadata().event_desc[1].attribute.type, perf_data::parser::attribute_type::hardware);
    EXPECT_EQ(file.metadata().event_desc[1].attribute.sample_period_freq, 5000);
    EXPECT_EQ(file.metadata().event_desc[1].attribute.sample_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000101100111"));
    EXPECT_EQ(file.metadata().event_desc[1].attribute.read_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000000010100"));
    EXPECT_EQ(file.metadata().event_desc[1].attribute.flags.data(), std::bitset<64>("0000000000000000000000000000000000000000000101000001010001100011"));
    EXPECT_EQ(file.metadata().event_desc[1].attribute.precise_ip, perf_data::parser::skid_constraint_type::can_have_arbitrary_skid);
    EXPECT_FALSE(file.metadata().event_desc[1].attribute.name.has_value());
    EXPECT_EQ(file.metadata().event_desc[1].event_string, "branch-misses:u");
    EXPECT_EQ(file.metadata().event_desc[1].ids, (std::vector<std::size_t>{46, 47, 48, 49, 50, 51, 52, 53}));

    EXPECT_EQ(file.metadata().event_desc[2].attribute.type, perf_data::parser::attribute_type::hardware);
    EXPECT_EQ(file.metadata().event_desc[2].attribute.sample_period_freq, 5000);
    EXPECT_EQ(file.metadata().event_desc[2].attribute.sample_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000101100111"));
    EXPECT_EQ(file.metadata().event_desc[2].attribute.read_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000000010100"));
    EXPECT_EQ(file.metadata().event_desc[2].attribute.flags.data(), std::bitset<64>("0000000000000000000000000000000000000000000101000001010001100011"));
    EXPECT_EQ(file.metadata().event_desc[2].attribute.precise_ip, perf_data::parser::skid_constraint_type::can_have_arbitrary_skid);
    EXPECT_FALSE(file.metadata().event_desc[2].attribute.name.has_value());
    EXPECT_EQ(file.metadata().event_desc[2].event_string, "cache-references:u");
    EXPECT_EQ(file.metadata().event_desc[2].ids, (std::vector<std::size_t>{54, 55, 56, 57, 58, 59, 60, 61}));

    EXPECT_EQ(file.metadata().event_desc[3].attribute.type, perf_data::parser::attribute_type::hardware);
    EXPECT_EQ(file.metadata().event_desc[3].attribute.sample_period_freq, 5000);
    EXPECT_EQ(file.metadata().event_desc[3].attribute.sample_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000101100111"));
    EXPECT_EQ(file.metadata().event_desc[3].attribute.read_format.data(), std::bitset<64>("0000000000000000000000000000000000000000000000000000000000010100"));
    EXPECT_EQ(file.metadata().event_desc[3].attribute.flags.data(), std::bitset<64>("0000000000000000000000000000000000000000000101000001010001100011"));
    EXPECT_EQ(file.metadata().event_desc[3].attribute.precise_ip, perf_data::parser::skid_constraint_type::can_have_arbitrary_skid);
    EXPECT_FALSE(file.metadata().event_desc[3].attribute.name.has_value());
    EXPECT_EQ(file.metadata().event_desc[3].event_string, "cache-misses:u");
    EXPECT_EQ(file.metadata().event_desc[3].ids, (std::vector<std::size_t>{62, 63, 64, 65, 66, 67, 68, 69}));

    EXPECT_EQ(file.metadata().sample_time->start, 17346006200ns);
    EXPECT_EQ(file.metadata().sample_time->end, 21211232100ns);
    EXPECT_FALSE(file.metadata().clockid.has_value());

    const std::map<perf_data::parser::event_type, std::size_t> expected_event_counts = {
        {perf_data::parser::event_type::comm,           2    },
        {perf_data::parser::event_type::exit,           1    },
        {perf_data::parser::event_type::sample,         65139},
        {perf_data::parser::event_type::mmap2,          7    },
        {perf_data::parser::event_type::finished_round, 68   },
        {perf_data::parser::event_type::id_index,       1    },
        {perf_data::parser::event_type::thread_map,     1    },
        {perf_data::parser::event_type::cpu_map,        1    },
        {perf_data::parser::event_type::finished_init,  1    }
    };

    EXPECT_EQ(counting_observer.counts, expected_event_counts);
}

TEST(PerfDataFile, ReadInnerCancel)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto file_path = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "linux" / "deb" / "record" / "inner-perf.data";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    perf_data::perf_data_file file(file_path);

    counting_event_observer counting_observer;

    test_progress_listener progress_listener(0.1);
    progress_listener.cancel_at = 0.2;

    file.process(counting_observer, &progress_listener, &progress_listener.token);

    EXPECT_TRUE(progress_listener.started);
    EXPECT_FALSE(progress_listener.finished);
    EXPECT_EQ(progress_listener.reported,
              (std::vector<double>{0.0, 10.0, 20.0}));

    const std::map<perf_data::parser::event_type, std::size_t> expected_event_counts = {
        {perf_data::parser::event_type::comm,          2  },
        {perf_data::parser::event_type::sample,        301},
        {perf_data::parser::event_type::mmap2,         7  },
        {perf_data::parser::event_type::id_index,      1  },
        {perf_data::parser::event_type::thread_map,    1  },
        {perf_data::parser::event_type::cpu_map,       1  },
        {perf_data::parser::event_type::finished_init, 1  }
    };

    EXPECT_EQ(counting_observer.counts, expected_event_counts);
}
