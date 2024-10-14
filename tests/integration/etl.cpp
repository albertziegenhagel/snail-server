
#include <gtest/gtest.h>

#include <cmath>

#include <folders.hpp>

#include <snail/etl/dispatching_event_observer.hpp>
#include <snail/etl/etl_file.hpp>

#include <snail/etl/parser/records/kernel_trace_control/image_id.hpp>
#include <snail/etl/parser/records/kernel_trace_control/system_config_ex.hpp>

#include <snail/etl/parser/records/visual_studio/diagnostics_hub.hpp>

using namespace snail;
using namespace snail::detail::tests;

namespace {

// std::string_view guid_to_string(const common::guid& guid)
// {
//     if(guid == etl::parser::system_config_ex_guid) return "system_config_ex_guid";
//     else if(guid == etl::parser::image_id_guid) return "image_id_guid";
//     else if(guid == etl::parser::vs_diagnostics_hub_guid) return "vs_diagnostics_hub_guid";
//     return "unknown";
// }

std::string_view group_to_string(etl::parser::event_trace_group group)
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

template<typename T>
    requires std::same_as<T, etl::parser::system_trace_header_view> ||
             std::same_as<T, etl::parser::compact_trace_header_view> ||
             std::same_as<T, etl::parser::perfinfo_trace_header_view>
auto make_key(const T& trace_header)
{
    return etl::detail::group_handler_key{
        trace_header.packet().group(),
        trace_header.packet().type(),
        trace_header.version()};
}

template<typename T>
    requires std::same_as<T, etl::parser::full_header_trace_header_view> ||
             std::same_as<T, etl::parser::instance_trace_header_view>
auto make_key(const T& trace_header)
{
    return etl::detail::guid_handler_key{
        trace_header.guid().instantiate(),
        trace_header.trace_class().type(),
        trace_header.trace_class().version()};
}

auto make_key(const etl::parser::event_header_trace_header_view& trace_header)
{
    return etl::detail::guid_handler_key{
        trace_header.provider_id().instantiate(),
        trace_header.event_descriptor().id(),
        trace_header.event_descriptor().version()};
}

void register_counting(etl::dispatching_event_observer&                                 counting_observer,
                       std::unordered_map<etl::detail::guid_handler_key, std::size_t>&  guid_event_counts,
                       std::unordered_map<etl::detail::group_handler_key, std::size_t>& group_event_counts)
{
    counting_observer.register_unknown_event(
        [&guid_event_counts]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                             const etl::any_guid_trace_header&                  header,
                             [[maybe_unused]] const std::span<const std::byte>& event_data)
        {
            const auto header_key = std::visit([](const auto& header)
                                               { return make_key(header); },
                                               header);
            ++guid_event_counts[header_key];
        });
    counting_observer.register_unknown_event(
        [&group_event_counts]([[maybe_unused]] const etl::etl_file::header_data& file_header,
                              const etl::any_group_trace_header&                 header,
                              [[maybe_unused]] const std::span<const std::byte>& event_data)
        {
            const auto header_key = std::visit([](const auto& header)
                                               { return make_key(header); },
                                               header);
            ++group_event_counts[header_key];
        });
}

struct test_progress_listener : public common::progress_listener
{
    using common::progress_listener::progress_listener;

    virtual void start() const override
    {
        started = true;
    }

    virtual void report(double progress) const override
    {
        reported.push_back(std::round(progress * 1000.0) / 10.0); // percent rounded to one digit after the point
        if(cancel_at && progress >= *cancel_at)
        {
            token.cancel();
        }
    }

    virtual void finish() const override
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

namespace snail::etl::detail {

std::ostream& operator<<(std::ostream& stream, const etl::detail::group_handler_key& key)
{
    stream << "{ " << group_to_string(key.group)
           << ", " << (int)key.type
           << ", " << key.version
           << " }";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const etl::detail::guid_handler_key& key)
{
    stream << "{ " << key.guid.to_string(true)
           << ", " << key.type
           << ", " << key.version
           << " }";
    return stream;
}

} // namespace snail::etl::detail

TEST(EtlFile, ReadInner)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto file_path = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "windows" / "deb" / "record" / "inner.etl";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    etl::etl_file file(file_path);

    EXPECT_EQ(file.header().start_time, common::nt_sys_time(common::nt_duration(16882128775211394)));
    EXPECT_EQ(file.header().end_time, common::nt_sys_time(common::nt_duration(16882128809863152)));
    EXPECT_EQ(file.header().start_time_qpc_ticks, 2901346334);
    EXPECT_EQ(file.header().qpc_frequency, 10000000);
    EXPECT_EQ(file.header().pointer_size, 8);
    EXPECT_EQ(file.header().number_of_processors, 2);
    EXPECT_EQ(file.header().number_of_buffers, 82);

    etl::dispatching_event_observer counting_observer;

    std::unordered_map<etl::detail::guid_handler_key, std::size_t>  guid_event_counts;
    std::unordered_map<etl::detail::group_handler_key, std::size_t> group_event_counts;

    register_counting(counting_observer, guid_event_counts, group_event_counts);

    const test_progress_listener progress_listener(0.1);

    file.process(counting_observer, &progress_listener);

    EXPECT_TRUE(progress_listener.started);
    EXPECT_TRUE(progress_listener.finished);
    EXPECT_EQ(progress_listener.reported,
              (std::vector<double>{0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}));

    const std::unordered_map<etl::detail::guid_handler_key, std::size_t> expected_guid_event_counts = {
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 0, 2},           7010},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 32, 2},          13  },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 34, 2},          3   },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 36, 2},          6994},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 37, 2},          310 },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 38, 2},          6831},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 40, 1},          6613},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 64, 0},          1331},
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 32, 0},  1   },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 33, 0},  1   },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 34, 0},  1   },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 35, 0},  9   },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 37, 0},  1   },
        {etl::detail::guid_handler_key{etl::parser::vs_diagnostics_hub_guid, 1, 2}, 1   },
        {etl::detail::guid_handler_key{etl::parser::vs_diagnostics_hub_guid, 5, 0}, 1   },
        {etl::detail::guid_handler_key{etl::parser::vs_diagnostics_hub_guid, 6, 0}, 25  }
    };

    const std::unordered_map<etl::detail::group_handler_key, std::size_t> expected_group_event_counts = {
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 0, 2},     1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 5, 2},     4   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 8, 2},     5   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 32, 2},    5   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 10, 3},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 11, 2},    2   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 12, 2},    2   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 13, 2},    3   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 15, 3},    236 },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 16, 2},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 21, 3},    12  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 22, 5},    72  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 23, 2},    2   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 25, 2},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 28, 2},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 29, 2},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 30, 2},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 31, 2},    2   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 33, 2},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 34, 2},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 35, 2},    2   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 36, 2},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 37, 2},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 1, 4},    3   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 2, 4},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 3, 4},    272 },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 4, 4},    135 },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 10, 3},   92  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 11, 2},   1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 39, 5},   14  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 1, 3},     55  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 2, 3},     9   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 3, 3},     2385},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 4, 3},     1204},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 2, 3},      20  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 3, 3},      494 },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 4, 3},      6413},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 33, 2},     1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 34, 2},     1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::perfinfo, 46, 2},  6886},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::perfinfo, 73, 3},  1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::perfinfo, 74, 3},  1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::stackwalk, 32, 2}, 646 }
    };

    EXPECT_EQ(guid_event_counts, expected_guid_event_counts);
    EXPECT_EQ(group_event_counts, expected_group_event_counts);
}

TEST(EtlFile, ReadOrdered)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto file_path = get_root_dir().value() / "tests" / "apps" / "ordered" / "dist" / "windows" / "deb" / "record" / "ordered_merged.etl";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    etl::etl_file file(file_path);

    EXPECT_EQ(file.header().start_time, common::nt_sys_time(common::nt_duration(17131983583517451)));
    EXPECT_EQ(file.header().end_time, common::nt_sys_time(common::nt_duration(17131983640883772)));
    EXPECT_EQ(file.header().start_time_qpc_ticks, 368418466025);
    EXPECT_EQ(file.header().qpc_frequency, 10000000);
    EXPECT_EQ(file.header().pointer_size, 8);
    EXPECT_EQ(file.header().number_of_processors, 8);
    EXPECT_EQ(file.header().number_of_buffers, 168);

    etl::dispatching_event_observer counting_observer;

    std::unordered_map<etl::detail::guid_handler_key, std::size_t>  guid_event_counts;
    std::unordered_map<etl::detail::group_handler_key, std::size_t> group_event_counts;

    register_counting(counting_observer, guid_event_counts, group_event_counts);

    file.process(counting_observer);

    const std::unordered_map<etl::detail::guid_handler_key, std::size_t> expected_guid_event_counts = {
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 0, 2},          7574},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 32, 2},         30  },
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 36, 2},         7544},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 38, 2},         7409},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 40, 1},         7383},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 64, 0},         883 },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 32, 0}, 1   },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 33, 0}, 1   },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 35, 0}, 11  },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 36, 0}, 6   },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 37, 0}, 1   },
    };

    const std::unordered_map<etl::detail::group_handler_key, std::size_t> expected_group_event_counts = {
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 10, 3},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 11, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 12, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 13, 2},    2    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 15, 3},    179  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 16, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 19, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 21, 3},    3    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 22, 5},    62   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 24, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 26, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 27, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 28, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 29, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 33, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 34, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 35, 2},    8    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 36, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::config, 37, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 0, 2},     1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 32, 2},    2    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 5, 2},     3    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 8, 2},     2    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 80, 2},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 2, 3},      64   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 3, 3},      3739 },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 33, 2},     1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 34, 2},     1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 4, 3},      3722 },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::perfinfo, 46, 2},  29246},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::perfinfo, 47, 2},  13669},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::perfinfo, 73, 3},  4    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::perfinfo, 74, 3},  4    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 1, 4},    1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 10, 3},   49   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 11, 2},   3    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 2, 4},    3    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 3, 4},    68   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 39, 5},   1    },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 4, 4},    66   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::stackwalk, 32, 2}, 32076},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 1, 3},     17   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 2, 3},     20   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 3, 3},     825  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 4, 3},     830  },
    };

    EXPECT_EQ(guid_event_counts, expected_guid_event_counts);
    EXPECT_EQ(group_event_counts, expected_group_event_counts);

    // std::cout << "START TIME " << file.header().start_time.time_since_epoch().count() << "\n";
    // std::cout << "END TIME " << file.header().end_time.time_since_epoch().count() << "\n";
    // for(const auto& [key, count] : guid_event_counts)
    // {
    //     std::cout << "{etl::detail::guid_handler_key{etl::parser::" << guid_to_string(key.guid) << ", " << (int)key.type << ", " << (int)key.version << "}, " << count << "},\n";
    // }
    // for(const auto& [key, count] : group_event_counts)
    // {
    //     std::cout << "{etl::detail::group_handler_key{etl::parser::event_trace_group::" << group_to_string(key.group) << ", " << (int)key.type << ", " << (int)key.version << "}, " << count << "},\n";
    // }
}

TEST(EtlFile, ReadInnerCancel)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto file_path = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "windows" / "deb" / "record" / "inner.etl";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    etl::etl_file file(file_path);

    etl::dispatching_event_observer counting_observer;

    std::unordered_map<etl::detail::guid_handler_key, std::size_t>  guid_event_counts;
    std::unordered_map<etl::detail::group_handler_key, std::size_t> group_event_counts;

    register_counting(counting_observer, guid_event_counts, group_event_counts);

    test_progress_listener progress_listener(0.1);
    progress_listener.cancel_at = 0.2;

    file.process(counting_observer, &progress_listener, &progress_listener.token);

    EXPECT_TRUE(progress_listener.started);
    EXPECT_FALSE(progress_listener.finished);
    EXPECT_EQ(progress_listener.reported,
              (std::vector<double>{0.0, 10.0, 20.0}));

    const std::unordered_map<etl::detail::guid_handler_key, std::size_t> expected_guid_event_counts = {
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 0, 2},           563},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 36, 2},          563},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 38, 2},          461},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 40, 1},          461},
        {etl::detail::guid_handler_key{etl::parser::image_id_guid, 64, 0},          284},
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 33, 0},  1  },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 34, 0},  1  },
        {etl::detail::guid_handler_key{etl::parser::system_config_ex_guid, 35, 0},  9  },
        {etl::detail::guid_handler_key{etl::parser::vs_diagnostics_hub_guid, 1, 2}, 1  },
        {etl::detail::guid_handler_key{etl::parser::vs_diagnostics_hub_guid, 6, 0}, 23 }
    };

    const std::unordered_map<etl::detail::group_handler_key, std::size_t> expected_group_event_counts = {
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 0, 2},     1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 5, 2},     2   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 8, 2},     3   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::header, 32, 2},    3   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 1, 4},    3   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 2, 4},    1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 3, 4},    272 },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 10, 3},   58  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::process, 11, 2},   1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 1, 3},     48  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 2, 3},     8   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::thread, 3, 3},     2385},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 2, 3},      17  },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::image, 3, 3},      494 },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::perfinfo, 46, 2},  6269},
        {etl::detail::group_handler_key{etl::parser::event_trace_group::perfinfo, 73, 3},  1   },
        {etl::detail::group_handler_key{etl::parser::event_trace_group::stackwalk, 32, 2}, 425 }
    };

    EXPECT_EQ(guid_event_counts, expected_guid_event_counts);
    EXPECT_EQ(group_event_counts, expected_group_event_counts);
}
