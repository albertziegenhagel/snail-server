
#include <gtest/gtest.h>

#include <cmath>
#include <ranges>

#include <folders.hpp>

#include <snail/analysis/diagsession_data_provider.hpp>
#include <snail/analysis/perf_data_data_provider.hpp>

#include <snail/common/system.hpp>

using namespace snail;
using namespace snail::detail::tests;

using namespace std::chrono_literals;

namespace {

template<std::ranges::range Range>
auto to_vector(Range&& range)
{
    std::vector<std::decay_t<std::ranges::range_value_t<Range>>> result;
    std::ranges::copy(range, std::back_inserter(result));
    return result;
}

std::string replace_all(std::string_view str, char search, char replace)
{
    auto result = std::string(str);
    std::ranges::replace(result, search, replace);
    return result;
}

struct test_progress_listener : public common::progress_listener
{
    using common::progress_listener::progress_listener;

    struct cancel_info
    {
        unsigned int run;
        double       progress;
    };

    virtual void start([[maybe_unused]] std::string_view                title,
                       [[maybe_unused]] std::optional<std::string_view> message) const override
    {
        ++started;
    }

    virtual void report(double                                           progress,
                        [[maybe_unused]] std::optional<std::string_view> message) const override
    {
        reported.push_back(std::round(progress * 1000.0) / 10.0); // percent rounded to one digit after the point
        if(cancel_at && started == cancel_at->run && progress >= cancel_at->progress)
        {
            token.cancel();
        }
    }

    virtual void finish([[maybe_unused]] std::optional<std::string_view> message) const override
    {
        ++finished;
    }

    mutable unsigned int               started  = 0;
    mutable unsigned int               finished = 0;
    mutable std::vector<double>        reported;
    mutable common::cancellation_token token;

    std::optional<cancel_info> cancel_at = std::nullopt;
};

} // namespace

namespace snail::analysis {

[[nodiscard]] bool operator==(const stack_frame& lhs, const stack_frame& rhs)
{
    return lhs.symbol_name == rhs.symbol_name &&
           lhs.module_name == rhs.module_name &&
           replace_all(lhs.file_path, '\\', '/') == rhs.file_path &&
           lhs.function_line_number == rhs.function_line_number &&
           lhs.instruction_line_number == rhs.instruction_line_number;
}

std::ostream& operator<<(std::ostream& stream, const analysis::stack_frame& frame)
{
    stream << "{ " << frame.symbol_name
           << ", " << frame.module_name
           << ", " << frame.file_path
           << ", " << frame.function_line_number
           << ", " << frame.instruction_line_number
           << " }";
    return stream;
}

} // namespace snail::analysis

TEST(DiagsessionDataProvider, ProcessInner)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto data_dir  = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "windows" / "deb";
    const auto file_path = data_dir / "record" / "inner.diagsession";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    // Disable all symbol downloading & caching
    analysis::pdb_symbol_find_options symbol_options;
    symbol_options.symbol_server_urls_.clear();
    symbol_options.symbol_cache_dir_ = common::get_temp_dir() / "should-not-exist-2f4f23da-ef58-4f0c-8f99-a83aed90fbbe";

    // Make sure the executable & pdb is found
    symbol_options.search_dirs_.clear();
    symbol_options.search_dirs_.push_back(data_dir / "bin");

    analysis::diagsession_data_provider data_provider(symbol_options);

    data_provider.process(file_path, nullptr, nullptr);

    EXPECT_EQ(data_provider.session_info().command_line, "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\Team Tools\\DiagnosticsHub\\Collector\\VSDiagnostics.exe\" start 8822D5E9-64DD-5269-B4F5-5387BD6C2FCB /launch:D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe /loadAgent:4EA90761-2248-496C-B854-3C0399A591A4;DiagnosticsHub.CpuAgent.dll ");
    EXPECT_EQ(data_provider.session_info().date, std::chrono::sys_seconds(1688212877s));
    EXPECT_EQ(data_provider.session_info().runtime, 3465175800ns);
    EXPECT_EQ(data_provider.session_info().number_of_processes, 1);
    EXPECT_EQ(data_provider.session_info().number_of_threads, 4);
    EXPECT_EQ(data_provider.session_info().number_of_samples, 292);

    EXPECT_EQ(data_provider.system_info().hostname, "fv-az448-164");
    EXPECT_EQ(data_provider.system_info().platform, "Windows Server 2022 Datacenter");
    EXPECT_EQ(data_provider.system_info().architecture, "x64");
    EXPECT_EQ(data_provider.system_info().cpu_name, "Intel(R) Xeon(R) Platinum 8272CL CPU @ 2.60GHz");
    EXPECT_EQ(data_provider.system_info().number_of_processors, 2);

    ASSERT_EQ(data_provider.sample_sources().size(), 1);
    const auto& sample_source = data_provider.sample_sources()[0];
    EXPECT_EQ(sample_source.name, "Timer");
    EXPECT_EQ(sample_source.number_of_samples, 292);
    EXPECT_EQ(sample_source.average_sampling_rate, 195.95397980519758);
    EXPECT_TRUE(sample_source.has_stacks);

    const auto sampling_processes = to_vector(data_provider.sampling_processes());
    ASSERT_EQ(sampling_processes.size(), 1);
    const auto unique_sampling_process_id = sampling_processes[0];

    const auto process_info = data_provider.process_info(unique_sampling_process_id);
    EXPECT_EQ(process_info.unique_id, unique_sampling_process_id);
    EXPECT_EQ(process_info.os_id, 4140);
    EXPECT_EQ(process_info.name, "inner.exe");
    EXPECT_EQ(process_info.start_time, 56312800ns);
    EXPECT_EQ(process_info.end_time, 2564215900ns);

    const auto threads = to_vector(data_provider.threads_info(unique_sampling_process_id));
    ASSERT_EQ(threads.size(), 4);

    const auto thread_4224_iter = std::ranges::find_if(threads, [](const analysis::thread_info& thread)
                                                       { return thread.os_id == 4224; });
    ASSERT_TRUE(thread_4224_iter != threads.end());
    const auto& thread_4224 = *thread_4224_iter;

    const auto thread_6180_iter = std::ranges::find_if(threads, [](const analysis::thread_info& thread)
                                                       { return thread.os_id == 6180; });
    ASSERT_TRUE(thread_6180_iter != threads.end());
    const auto& thread_6180 = *thread_6180_iter;

    const auto thread_3828_iter = std::ranges::find_if(threads, [](const analysis::thread_info& thread)
                                                       { return thread.os_id == 3828; });
    ASSERT_TRUE(thread_3828_iter != threads.end());
    const auto& thread_3828 = *thread_3828_iter;

    const auto thread_3148_iter = std::ranges::find_if(threads, [](const analysis::thread_info& thread)
                                                       { return thread.os_id == 3148; });
    ASSERT_TRUE(thread_3148_iter != threads.end());
    const auto& thread_3148 = *thread_3148_iter;

    EXPECT_EQ(thread_4224.os_id, 4224);
    EXPECT_EQ(thread_4224.name, std::nullopt);
    EXPECT_EQ(thread_4224.start_time, 1138034300ns);
    EXPECT_EQ(thread_4224.end_time, 2563493400ns);

    EXPECT_EQ(thread_6180.os_id, 6180);
    EXPECT_EQ(thread_6180.name, std::nullopt);
    EXPECT_EQ(thread_6180.start_time, 1137662800ns);
    EXPECT_EQ(thread_6180.end_time, 2563540200ns);

    EXPECT_EQ(thread_3828.os_id, 3828);
    EXPECT_EQ(thread_3828.name, std::nullopt);
    EXPECT_EQ(thread_3828.start_time, 56313900ns);
    EXPECT_EQ(thread_3828.end_time, 2563993900ns);

    EXPECT_EQ(thread_3148.os_id, 3148);
    EXPECT_EQ(thread_3148.name, std::nullopt);
    EXPECT_EQ(thread_3148.start_time, 1137927400ns);
    EXPECT_EQ(thread_3148.end_time, 2563524800ns);

    analysis::sample_filter  filter;
    std::chrono::nanoseconds last_timestamp;
    std::size_t              sample_count;

    // We just compare some selected sample stacks
    const std::unordered_map<std::size_t, std::vector<analysis::stack_frame>> expected_sample_stacks = {
        {3,
         {{"ntdll.dll!0x00007ffde3a5799e", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3a7d9bd", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3a57b98", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3ab785d", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3a4e593", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"apphelp.dll!0x00007ffddf18bfb5", "C:\\Windows\\System32\\apphelp.dll", "", 0, 0},
          {"apphelp.dll!0x00007ffddf182c85", "C:\\Windows\\System32\\apphelp.dll", "", 0, 0},
          {"apphelp.dll!0x00007ffddf182e26", "C:\\Windows\\System32\\apphelp.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3a1e8f4", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde39f8e3c", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde39fbf5a", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde39fcdfd", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde39f470d", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde39f47fb", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde39fc4f9", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde39fff76", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0}}                                                                                                                                                                                                                                     },
        {7,
         {{"ntdll.dll!0x00007ffde3a5e44b", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"kernel32.dll!0x00007ffde2e54de0", "C:\\Windows\\System32\\kernel32.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde39ebb26", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3a48c2a", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3a1b19b", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3a1b321", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3a1bce4", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3a1bf63", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntdll.dll!0x00007ffde3a7fe24", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483a33b85", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483cd5a3c", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483cf07dc", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483cf0ab4", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483cf14eb", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483ce55d4", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483ce67aa", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483ce4684", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483ce4138", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483a34ece", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483a2fb41", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483895c32", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff804838972d5", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483899a2f", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff804838f3203", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff804838f45d3", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff804838f54b5", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff8048389eabf", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff8048389f531", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483a29227", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0}}                                                                                                                                                                                                                               },
        {39,
         {{"ntdll.dll!0x00007ffde3a5e44b", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"kernel32.dll!0x00007ffde2e54de0", "C:\\Windows\\System32\\kernel32.dll", "", 0, 0},
          {"mainCRTStartup", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_main.cpp", 15, 17},
          {"__scrt_common_main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 324, 331},
          {"__scrt_common_main_seh", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 236, 288},
          {"invoke_main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 77, 79},
          {"main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/snail-server/snail-server/tests/apps/inner/main.cpp", 58, 70},
          {"void __cdecl make_random_vector(class std::vector<double, class std::allocator<double>> &, unsigned __int64)", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/snail-server/snail-server/tests/apps/inner/main.cpp", 12, 20}}                                                          },
        {96,
         {{"ntoskrnl.exe!0xfffff80483a21be1", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483a27c05", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483a28510", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483a28725", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483838314", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff804838399cc", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff8048383c9e5", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0},
          {"ntoskrnl.exe!0xfffff80483835de6", "C:\\Windows\\system32\\ntoskrnl.exe", "", 0, 0}}                                                                                                                                                                                                                               },
        {283,
         {{"ntdll.dll!0x00007ffde3a5e44b", "C:\\Windows\\System32\\ntdll.dll", "", 0, 0},
          {"kernel32.dll!0x00007ffde2e54de0", "C:\\Windows\\System32\\kernel32.dll", "", 0, 0},
          {"mainCRTStartup", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_main.cpp", 15, 17},
          {"__scrt_common_main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 324, 331},
          {"__scrt_common_main_seh", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 236, 288},
          {"invoke_main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 77, 79},
          {"main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/snail-server/snail-server/tests/apps/inner/main.cpp", 58, 72},
          {"double __cdecl compute_inner_product(class std::vector<double, class std::allocator<double>> const &, class std::vector<double, class std::allocator<double>> const &)", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/snail-server/snail-server/tests/apps/inner/main.cpp", 28, 37}}}
    };

    // No filter
    sample_count   = 0;
    last_timestamp = 0ns;
    for(const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1;
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 292);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 292);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_4224.unique_id, filter), 3);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_6180.unique_id, filter), 2);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3828.unique_id, filter), 286);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3148.unique_id, filter), 1);

    // Filter complete range
    filter.min_time = 1073706700ns;
    filter.max_time = 2563852500ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1;
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 292);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 292);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_4224.unique_id, filter), 3);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_6180.unique_id, filter), 2);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3828.unique_id, filter), 286);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3148.unique_id, filter), 1);

    // Filter first half
    filter.min_time = std::nullopt;
    filter.max_time = 1818779600ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1;
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 11);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 11);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_4224.unique_id, filter), 2);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_6180.unique_id, filter), 2);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3828.unique_id, filter), 6);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3148.unique_id, filter), 1);

    // Filter second half
    filter.min_time = 1818779600ns;
    filter.max_time = std::nullopt;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1 + 11;
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 281);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 281);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_4224.unique_id, filter), 1);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_6180.unique_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3828.unique_id, filter), 280);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3148.unique_id, filter), 0);

    // Filter somewhere in the middle
    filter.min_time = 2200000000ns;
    filter.max_time = 2300000000ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1 + 102;
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 99);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 99);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_4224.unique_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_6180.unique_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3828.unique_id, filter), 99);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3148.unique_id, filter), 0);

    // None empty
    filter.min_time = 2300000000ns;
    filter.max_time = 2200000000ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;
    }
    EXPECT_EQ(sample_count, 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_4224.unique_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_6180.unique_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3828.unique_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3148.unique_id, filter), 0);

    // Filter one unimportant thread
    filter.min_time = std::nullopt;
    filter.max_time = std::nullopt;
    filter.excluded_threads.clear();
    filter.excluded_threads.insert(thread_3148.unique_id);
    sample_count   = 0;
    last_timestamp = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1 + (sample_count > 6 ? 1 : 0);
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 291);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 291);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_4224.unique_id, filter), 3);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_6180.unique_id, filter), 2);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3828.unique_id, filter), 286);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3148.unique_id, filter), 0);

    // Filter the main thread and another one
    filter.min_time = std::nullopt;
    filter.max_time = std::nullopt;
    filter.excluded_threads.clear();
    filter.excluded_threads.insert(thread_3828.unique_id);
    filter.excluded_threads.insert(thread_4224.unique_id);
    sample_count   = 0;
    last_timestamp = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;
    }
    EXPECT_EQ(sample_count, 3);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 3);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_4224.unique_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_6180.unique_id, filter), 2);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3828.unique_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3148.unique_id, filter), 1);

    // Filter the process
    filter.min_time = std::nullopt;
    filter.max_time = std::nullopt;
    filter.excluded_threads.clear();
    filter.excluded_processes.insert(unique_sampling_process_id);
    sample_count   = 0;
    last_timestamp = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;
    }
    EXPECT_EQ(sample_count, 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_4224.unique_id, filter), 3);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_6180.unique_id, filter), 2);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3828.unique_id, filter), 286);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, thread_3148.unique_id, filter), 1);
}

TEST(DiagsessionDataProvider, ProcessInnerCancel)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto data_dir  = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "windows" / "deb";
    const auto file_path = data_dir / "record" / "inner.diagsession";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    // Disable all symbol downloading & caching
    analysis::pdb_symbol_find_options symbol_options;
    symbol_options.symbol_server_urls_.clear();
    symbol_options.symbol_cache_dir_ = common::get_temp_dir() / "should-not-exist-2f4f23da-ef58-4f0c-8f99-a83aed90fbbe";

    // Make sure the executable & pdb is found
    symbol_options.search_dirs_.clear();
    symbol_options.search_dirs_.push_back(data_dir / "bin");

    {
        // Do not cancel at all
        analysis::diagsession_data_provider data_provider(symbol_options);

        const test_progress_listener progress_listener(0.1);

        data_provider.process(file_path, &progress_listener, nullptr);

        EXPECT_EQ(progress_listener.started, 2);
        EXPECT_EQ(progress_listener.finished, 2);
        EXPECT_EQ(progress_listener.reported,
                  (std::vector<double>{0.0, 19.5, 29.3, 39, 48.8, 58.5, 68.3, 78, 87.8, 97.6, 100.0,
                                       0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}));
    }
    {
        // Cancel during extraction of the diagsession file
        analysis::diagsession_data_provider data_provider(symbol_options);

        test_progress_listener progress_listener(0.1);
        progress_listener.cancel_at = test_progress_listener::cancel_info{
            .run      = 1,
            .progress = 0.2};

        data_provider.process(file_path, &progress_listener, &progress_listener.token);

        EXPECT_EQ(progress_listener.started, 1);
        EXPECT_EQ(progress_listener.finished, 0);
        EXPECT_EQ(progress_listener.reported,
                  (std::vector<double>{0.0, 19.5, 29.3}));
    }

    {
        // Cancel while processing extracted ETL file
        analysis::diagsession_data_provider data_provider(symbol_options);

        test_progress_listener progress_listener(0.1);
        progress_listener.cancel_at = test_progress_listener::cancel_info{
            .run      = 2,
            .progress = 0.2};

        data_provider.process(file_path, &progress_listener, &progress_listener.token);

        EXPECT_EQ(progress_listener.started, 2);
        EXPECT_EQ(progress_listener.finished, 1);
        EXPECT_EQ(progress_listener.reported,
                  (std::vector<double>{0.0, 19.5, 29.3, 39, 48.8, 58.5, 68.3, 78, 87.8, 97.6, 100.0,
                                       0.0, 10.0, 20.0}));
    }
}

TEST(EtlDataProvider, ProcessOrdered)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto data_dir  = get_root_dir().value() / "tests" / "apps" / "ordered" / "dist" / "windows" / "deb";
    const auto file_path = data_dir / "record" / "ordered_merged.etl";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    // Disable all symbol downloading & caching
    analysis::pdb_symbol_find_options symbol_options;
    symbol_options.symbol_server_urls_.clear();
    symbol_options.symbol_cache_dir_ = common::get_temp_dir() / "should-not-exist-2f4f23da-ef58-4f0c-8f99-a83aed90fbbe";

    // Make sure the executable & pdb is found
    symbol_options.search_dirs_.clear();
    symbol_options.search_dirs_.push_back(data_dir / "bin");

    analysis::etl_data_provider data_provider(symbol_options);

    data_provider.process(file_path, nullptr, nullptr);

    EXPECT_EQ(data_provider.session_info().command_line, "\"C:\\Program Files (x86)\\Windows Kits\\10\\Windows Performance Toolkit\\xperf.exe\" -SetProfInt 2000 -on PROC_THREAD+LOADER+PMC_PROFILE+PROFILE -PmcProfile BranchMispredictions,LLCReference,LLCMisses -stackwalk Profile -PidNewProcess \"C:\\data\\sandbox\\ordered\\dist\\windows\\deb\\bin\\ordered.exe 10000000\" -WaitForNewProcess -f ordered.etl");
    EXPECT_EQ(data_provider.session_info().date, std::chrono::sys_seconds(1713198358s));
    EXPECT_EQ(data_provider.session_info().runtime, 5736632100ns);
    EXPECT_EQ(data_provider.session_info().number_of_processes, 25);
    EXPECT_EQ(data_provider.session_info().number_of_threads, 519);
    EXPECT_EQ(data_provider.session_info().number_of_samples, 41920);

    EXPECT_EQ(data_provider.system_info().hostname, "EE50C3DA-EC93-4");
    EXPECT_EQ(data_provider.system_info().platform, "Windows 10 Enterprise");
    EXPECT_EQ(data_provider.system_info().architecture, "x64");
    EXPECT_EQ(data_provider.system_info().cpu_name, "Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz");
    EXPECT_EQ(data_provider.system_info().number_of_processors, 8);

    ASSERT_EQ(data_provider.sample_sources().size(), 4);
    const auto& sample_source_0 = data_provider.sample_sources()[0];
    EXPECT_EQ(sample_source_0.name, "Timer");
    EXPECT_EQ(sample_source_0.number_of_samples, 28439);
    EXPECT_EQ(sample_source_0.average_sampling_rate, 5025.1791709883873);
    EXPECT_TRUE(sample_source_0.has_stacks);
    const auto& sample_source_1 = data_provider.sample_sources()[1];
    EXPECT_EQ(sample_source_1.name, "BranchMispredictions");
    EXPECT_EQ(sample_source_1.number_of_samples, 393);
    EXPECT_EQ(sample_source_1.average_sampling_rate, 72.165883253669918);
    EXPECT_FALSE(sample_source_1.has_stacks);
    const auto& sample_source_2 = data_provider.sample_sources()[2];
    EXPECT_EQ(sample_source_2.name, "LLCReference");
    EXPECT_EQ(sample_source_2.number_of_samples, 9616);
    EXPECT_EQ(sample_source_2.average_sampling_rate, 1699.7192776447603);
    EXPECT_FALSE(sample_source_2.has_stacks);
    const auto& sample_source_3 = data_provider.sample_sources()[3];
    EXPECT_EQ(sample_source_3.name, "LLCMisses");
    EXPECT_EQ(sample_source_3.number_of_samples, 3472);
    EXPECT_EQ(sample_source_3.average_sampling_rate, 619.66920729851813);
    EXPECT_FALSE(sample_source_3.has_stacks);

    const auto sampling_processes = to_vector(data_provider.sampling_processes());
    EXPECT_EQ(sampling_processes.size(), 25);
    const auto proccess_4692_iter = std::ranges::find_if(sampling_processes, [&data_provider](analysis::unique_process_id id)
                                                         { return data_provider.process_info(id).os_id == 4692; });
    ASSERT_TRUE(proccess_4692_iter != sampling_processes.end());

    const auto unique_sampling_process_id = *proccess_4692_iter;

    const auto process_info = data_provider.process_info(unique_sampling_process_id);
    EXPECT_EQ(process_info.unique_id, unique_sampling_process_id);
    EXPECT_EQ(process_info.os_id, 4692);
    EXPECT_EQ(process_info.name, "ordered.exe");
    EXPECT_EQ(process_info.start_time, 229926800ns);
    EXPECT_EQ(process_info.end_time, 4628548400ns);

    const auto threads = to_vector(data_provider.threads_info(unique_sampling_process_id));
    ASSERT_EQ(threads.size(), 4);

    const auto thread_5844_iter = std::ranges::find_if(threads, [](const analysis::thread_info& thread)
                                                       { return thread.os_id == 5844; });
    ASSERT_TRUE(thread_5844_iter != threads.end());
    const auto& thread_5844 = *thread_5844_iter;

    const auto thread_5852_iter = std::ranges::find_if(threads, [](const analysis::thread_info& thread)
                                                       { return thread.os_id == 5852; });
    ASSERT_TRUE(thread_5852_iter != threads.end());
    const auto& thread_5852 = *thread_5852_iter;

    const auto thread_2140_iter = std::ranges::find_if(threads, [](const analysis::thread_info& thread)
                                                       { return thread.os_id == 2140; });
    ASSERT_TRUE(thread_2140_iter != threads.end());
    const auto& thread_2140 = *thread_2140_iter;

    const auto thread_1704_iter = std::ranges::find_if(threads, [](const analysis::thread_info& thread)
                                                       { return thread.os_id == 1704; });
    ASSERT_TRUE(thread_1704_iter != threads.end());
    const auto& thread_1704 = *thread_1704_iter;

    EXPECT_EQ(thread_5844.os_id, 5844);
    EXPECT_EQ(thread_5844.name, std::nullopt);
    EXPECT_EQ(thread_5844.start_time, 229928900ns);
    EXPECT_EQ(thread_5844.end_time, 4628091500ns);

    EXPECT_EQ(thread_5852.os_id, 5852);
    EXPECT_EQ(thread_5852.name, std::nullopt);
    EXPECT_EQ(thread_5852.start_time, 237485400ns);
    EXPECT_EQ(thread_5852.end_time, 4625613800ns);

    EXPECT_EQ(thread_2140.os_id, 2140);
    EXPECT_EQ(thread_2140.name, std::nullopt);
    EXPECT_EQ(thread_2140.start_time, 237709500ns);
    EXPECT_EQ(thread_2140.end_time, 4624691400ns);

    EXPECT_EQ(thread_1704.os_id, 1704);
    EXPECT_EQ(thread_1704.name, std::nullopt);
    EXPECT_EQ(thread_1704.start_time, 237956000ns);
    EXPECT_EQ(thread_1704.end_time, 4624748300ns);

    analysis::sample_filter filter;

    EXPECT_EQ(data_provider.count_samples(sample_source_0.id, unique_sampling_process_id, filter), 21501);
    EXPECT_EQ(data_provider.count_samples(sample_source_0.id, thread_5844.unique_id, filter), 21452);
    EXPECT_EQ(data_provider.count_samples(sample_source_0.id, thread_5852.unique_id, filter), 22);
    EXPECT_EQ(data_provider.count_samples(sample_source_0.id, thread_2140.unique_id, filter), 14);
    EXPECT_EQ(data_provider.count_samples(sample_source_0.id, thread_1704.unique_id, filter), 13);

    EXPECT_EQ(data_provider.count_samples(sample_source_1.id, unique_sampling_process_id, filter), 257);
    EXPECT_EQ(data_provider.count_samples(sample_source_1.id, thread_5844.unique_id, filter), 255);
    EXPECT_EQ(data_provider.count_samples(sample_source_1.id, thread_5852.unique_id, filter), 1);
    EXPECT_EQ(data_provider.count_samples(sample_source_1.id, thread_2140.unique_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source_1.id, thread_1704.unique_id, filter), 1);

    EXPECT_EQ(data_provider.count_samples(sample_source_2.id, unique_sampling_process_id, filter), 7303);
    EXPECT_EQ(data_provider.count_samples(sample_source_2.id, thread_5844.unique_id, filter), 7269);
    EXPECT_EQ(data_provider.count_samples(sample_source_2.id, thread_5852.unique_id, filter), 13);
    EXPECT_EQ(data_provider.count_samples(sample_source_2.id, thread_2140.unique_id, filter), 11);
    EXPECT_EQ(data_provider.count_samples(sample_source_2.id, thread_1704.unique_id, filter), 10);

    EXPECT_EQ(data_provider.count_samples(sample_source_3.id, unique_sampling_process_id, filter), 3059);
    EXPECT_EQ(data_provider.count_samples(sample_source_3.id, thread_5844.unique_id, filter), 3048);
    EXPECT_EQ(data_provider.count_samples(sample_source_3.id, thread_5852.unique_id, filter), 4);
    EXPECT_EQ(data_provider.count_samples(sample_source_3.id, thread_2140.unique_id, filter), 4);
    EXPECT_EQ(data_provider.count_samples(sample_source_3.id, thread_1704.unique_id, filter), 3);
}

TEST(EtlDataProvider, ProcessOrderedCancel)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto data_dir  = get_root_dir().value() / "tests" / "apps" / "ordered" / "dist" / "windows" / "deb";
    const auto file_path = data_dir / "record" / "ordered_merged.etl";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    // Disable all symbol downloading & caching
    analysis::pdb_symbol_find_options symbol_options;
    symbol_options.symbol_server_urls_.clear();
    symbol_options.symbol_cache_dir_ = common::get_temp_dir() / "should-not-exist-2f4f23da-ef58-4f0c-8f99-a83aed90fbbe";

    // Make sure the executable & pdb is found
    symbol_options.search_dirs_.clear();
    symbol_options.search_dirs_.push_back(data_dir / "bin");

    {
        // Do not cancel at all
        analysis::etl_data_provider data_provider(symbol_options);

        const test_progress_listener progress_listener(0.1);

        data_provider.process(file_path, &progress_listener, nullptr);

        EXPECT_EQ(progress_listener.started, 1);
        EXPECT_EQ(progress_listener.finished, 1);
        EXPECT_EQ(progress_listener.reported,
                  (std::vector<double>{0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}));
    }
    {
        // Cancel during ETL file processing
        analysis::etl_data_provider data_provider(symbol_options);

        test_progress_listener progress_listener(0.1);
        progress_listener.cancel_at = test_progress_listener::cancel_info{
            .run      = 1,
            .progress = 0.2};

        data_provider.process(file_path, &progress_listener, &progress_listener.token);

        EXPECT_EQ(progress_listener.started, 1);
        EXPECT_EQ(progress_listener.finished, 0);
        EXPECT_EQ(progress_listener.reported,
                  (std::vector<double>{0.0, 10.0, 20.0}));
    }
}

TEST(PerfDataDataProvider, ProcessInner)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto data_dir  = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "linux" / "deb";
    const auto file_path = data_dir / "record" / "inner-perf.data";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    // Disable all symbol downloading & caching
    analysis::dwarf_symbol_find_options symbol_options;
    symbol_options.debuginfod_urls_.clear();
    symbol_options.debuginfod_cache_dir_ = common::get_temp_dir() / "should-not-exist-2f4f23da-ef58-4f0c-8f99-a83aed90fbbe";

    // Make sure the executable with debug info is found
    symbol_options.search_dirs_.clear();
    symbol_options.search_dirs_.push_back(data_dir / "bin");

    analysis::perf_data_data_provider data_provider(symbol_options);

    data_provider.process(file_path, nullptr, nullptr);

    EXPECT_EQ(data_provider.session_info().command_line, "/usr/bin/perf record -g -o inner-perf.data /tmp/build/inner/Debug/build/inner");
    // EXPECT_EQ(data_provider.session_info().date, std::chrono::sys_seconds(1688327245s)); // This is not stable
    EXPECT_EQ(data_provider.session_info().runtime, 387398400ns);
    EXPECT_EQ(data_provider.session_info().number_of_processes, 1);
    EXPECT_EQ(data_provider.session_info().number_of_threads, 1);
    EXPECT_EQ(data_provider.session_info().number_of_samples, 1524);

    EXPECT_EQ(data_provider.system_info().hostname, "DESKTOP-0RH3TEF");
    EXPECT_EQ(data_provider.system_info().platform, "5.15.90.1-microsoft-standard-WSL2");
    EXPECT_EQ(data_provider.system_info().architecture, "x86_64");
    EXPECT_EQ(data_provider.system_info().cpu_name, "Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz");
    EXPECT_EQ(data_provider.system_info().number_of_processors, 8);

    ASSERT_EQ(data_provider.sample_sources().size(), 1);
    const auto& sample_source = data_provider.sample_sources()[0];
    EXPECT_EQ(sample_source.name, "cycles:u");
    EXPECT_EQ(sample_source.number_of_samples, 1524);
    EXPECT_EQ(sample_source.average_sampling_rate, 3933.9346780988258);
    EXPECT_TRUE(sample_source.has_stacks);

    const auto sampling_processes = to_vector(data_provider.sampling_processes());
    ASSERT_EQ(sampling_processes.size(), 1);
    const auto unique_sampling_process_id = sampling_processes[0];

    const auto process_info = data_provider.process_info(unique_sampling_process_id);
    EXPECT_EQ(process_info.unique_id, unique_sampling_process_id);
    EXPECT_EQ(process_info.os_id, 248);
    EXPECT_EQ(process_info.name, "inner");
    EXPECT_EQ(process_info.start_time, 0ns);
    EXPECT_EQ(process_info.end_time, 387398400ns);

    const auto threads = to_vector(data_provider.threads_info(unique_sampling_process_id));
    ASSERT_EQ(threads.size(), 1);

    EXPECT_EQ(threads[0].os_id, 248);
    EXPECT_EQ(threads[0].name, "inner");
    EXPECT_EQ(threads[0].start_time, 0ns);
    EXPECT_EQ(threads[0].end_time, 387398400ns);

    analysis::sample_filter  filter;
    std::chrono::nanoseconds last_timestamp;
    std::size_t              sample_count;

    // We just compare some selected sample stacks
    const std::unordered_map<std::size_t, std::vector<analysis::stack_frame>> expected_sample_stacks = {
        {6,
         {{"ld-linux-x86-64.so.2!0x00007f6062e6e3b0", "/usr/lib64/ld-linux-x86-64.so.2", "", 0, 0},
          {"0xffffffff82000b40", "[unknown]", "", 0, 0}}                                                                                                                                                                                 },
        {735,
         {{"inner!0x0000000000402255", "/tmp/build/inner/Debug/build/inner", "", 0, 0},
          {"libc.so.6!0x00007f6062938c0b", "/usr/lib64/libc.so.6", "", 0, 0},
          {"libc.so.6!0x00007f6062938b4a", "/usr/lib64/libc.so.6", "", 0, 0},
          {"main", "/tmp/build/inner/Debug/build/inner", "/tmp/snail-server/tests/apps/inner/main.cpp", 57, 70},
          {"make_random_vector(std::vector<double, std::allocator<double>>&, unsigned long)", "/tmp/build/inner/Debug/build/inner", "/tmp/snail-server/tests/apps/inner/main.cpp", 11, 22}}                                              },
        {1502,
         {{"inner!0x0000000000402255", "/tmp/build/inner/Debug/build/inner", "", 0, 0},
          {"libc.so.6!0x00007f6062938c0b", "/usr/lib64/libc.so.6", "", 0, 0},
          {"libc.so.6!0x00007f6062938b4a", "/usr/lib64/libc.so.6", "", 0, 0},
          {"main", "/tmp/build/inner/Debug/build/inner", "/tmp/snail-server/tests/apps/inner/main.cpp", 57, 72},
          {"compute_inner_product(std::vector<double, std::allocator<double>> const&, std::vector<double, std::allocator<double>> const&)", "/tmp/build/inner/Debug/build/inner", "/tmp/snail-server/tests/apps/inner/main.cpp", 26, 36}}}
    };

    // No filter
    sample_count   = 0;
    last_timestamp = 0ns;
    for(const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        // Samples need to be ordered
        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1;
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 1524);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 1524);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, threads[0].unique_id, filter), 1524);

    // Filter complete range
    filter.min_time = 0ns;
    filter.max_time = 387398400ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1;
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 1524);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 1524);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, threads[0].unique_id, filter), 1524);

    // Filter first half
    filter.min_time = std::nullopt;
    filter.max_time = 193699200ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1;
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 755);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 755);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, threads[0].unique_id, filter), 755);

    // Filter second half
    filter.min_time = 193699200ns;
    filter.max_time = std::nullopt;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1 + 755;
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 769);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 769);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, threads[0].unique_id, filter), 769);

    // Filter somewhere in the middle
    filter.min_time = 180000000ns;
    filter.max_time = 200000000ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;

        const auto sample_index = sample_count - 1 + 700;
        if(!expected_sample_stacks.contains(sample_index)) continue;

        const auto expected_stack = expected_sample_stacks.at(sample_index);

        const auto stack = to_vector(sample.reversed_stack());

        EXPECT_EQ(stack, expected_stack);
    }
    EXPECT_EQ(sample_count, 80);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 80);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, threads[0].unique_id, filter), 80);

    // None empty
    filter.min_time = 230000000ns;
    filter.max_time = 220000000ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;
    }
    EXPECT_EQ(sample_count, 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, threads[0].unique_id, filter), 0);

    // Filter the only thread
    filter.min_time = std::nullopt;
    filter.max_time = std::nullopt;
    filter.excluded_threads.clear();
    filter.excluded_threads.insert(threads[0].unique_id);
    sample_count   = 0;
    last_timestamp = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;
    }
    EXPECT_EQ(sample_count, 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, threads[0].unique_id, filter), 0);

    // Filter the process
    filter.min_time = std::nullopt;
    filter.max_time = std::nullopt;
    filter.excluded_threads.clear();
    filter.excluded_processes.insert(unique_sampling_process_id);
    sample_count   = 0;
    last_timestamp = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(sample_source.id, unique_sampling_process_id, filter))
    {
        ++sample_count;
    }
    EXPECT_EQ(sample_count, 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, unique_sampling_process_id, filter), 0);
    EXPECT_EQ(data_provider.count_samples(sample_source.id, threads[0].unique_id, filter), 1524);
}

TEST(PerfDataDataProvider, ProcessInnerCancel)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto data_dir  = get_root_dir().value() / "tests" / "apps" / "inner" / "dist" / "linux" / "deb";
    const auto file_path = data_dir / "record" / "inner-perf.data";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    // Disable all symbol downloading & caching
    analysis::dwarf_symbol_find_options symbol_options;
    symbol_options.debuginfod_urls_.clear();
    symbol_options.debuginfod_cache_dir_ = common::get_temp_dir() / "should-not-exist-2f4f23da-ef58-4f0c-8f99-a83aed90fbbe";

    // Make sure the executable with debug info is found
    symbol_options.search_dirs_.clear();
    symbol_options.search_dirs_.push_back(data_dir / "bin");

    {
        // Do not cancel at all
        analysis::perf_data_data_provider data_provider(symbol_options);

        const test_progress_listener progress_listener(0.1);

        data_provider.process(file_path, &progress_listener, nullptr);

        EXPECT_EQ(progress_listener.started, 1);
        EXPECT_EQ(progress_listener.finished, 1);
        EXPECT_EQ(progress_listener.reported,
                  (std::vector<double>{0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}));
    }
    {
        // Cancel during perf-data file processing
        analysis::perf_data_data_provider data_provider(symbol_options);

        test_progress_listener progress_listener(0.1);
        progress_listener.cancel_at = test_progress_listener::cancel_info{
            .run      = 1,
            .progress = 0.2};

        data_provider.process(file_path, &progress_listener, &progress_listener.token);

        EXPECT_EQ(progress_listener.started, 1);
        EXPECT_EQ(progress_listener.finished, 0);
        EXPECT_EQ(progress_listener.reported,
                  (std::vector<double>{0.0, 10.0, 20.0}));
    }
}

TEST(PerfDataDataProvider, ProcessOrdered)
{
    ASSERT_TRUE(get_root_dir().has_value()) << "Missing root dir. Did you forget to pass --snail-root-dir=<dir> to the test executable?";
    const auto data_dir  = get_root_dir().value() / "tests" / "apps" / "ordered" / "dist" / "linux" / "deb";
    const auto file_path = data_dir / "record" / "ordered-perf.data";
    ASSERT_TRUE(std::filesystem::exists(file_path)) << "Missing test file:\n  " << file_path << "\nDid you forget checking out GIT LFS files?";

    // Disable all symbol downloading & caching
    analysis::dwarf_symbol_find_options symbol_options;
    symbol_options.debuginfod_urls_.clear();
    symbol_options.debuginfod_cache_dir_ = common::get_temp_dir() / "should-not-exist-2f4f23da-ef58-4f0c-8f99-a83aed90fbbe";

    // Make sure the executable with debug info is found
    symbol_options.search_dirs_.clear();
    symbol_options.search_dirs_.push_back(data_dir / "bin");

    analysis::perf_data_data_provider data_provider(symbol_options);

    data_provider.process(file_path, nullptr, nullptr);

    EXPECT_EQ(data_provider.session_info().command_line, "/usr/bin/perf record -g -e cycles,branch-misses,cache-references,cache-misses -F 5000 -o ordered-perf.data /tmp/build/ordered/Debug/build/ordered");
    // EXPECT_EQ(data_provider.session_info().date, std::chrono::sys_seconds(1688327245s)); // This is not stable
    EXPECT_EQ(data_provider.session_info().runtime, 3865225900ns);
    EXPECT_EQ(data_provider.session_info().number_of_processes, 1);
    EXPECT_EQ(data_provider.session_info().number_of_threads, 1);
    EXPECT_EQ(data_provider.session_info().number_of_samples, 65139);

    EXPECT_EQ(data_provider.system_info().hostname, "DESKTOP-0RH3TEF");
    EXPECT_EQ(data_provider.system_info().platform, "5.15.146.1-microsoft-standard-WSL2");
    EXPECT_EQ(data_provider.system_info().architecture, "x86_64");
    EXPECT_EQ(data_provider.system_info().cpu_name, "Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz");
    EXPECT_EQ(data_provider.system_info().number_of_processors, 8);

    ASSERT_EQ(data_provider.sample_sources().size(), 4);
    const auto& sample_source_0 = data_provider.sample_sources()[0];
    EXPECT_EQ(sample_source_0.name, "cache-references:u");
    EXPECT_EQ(sample_source_0.number_of_samples, 18603);
    EXPECT_EQ(sample_source_0.average_sampling_rate, 4812.9501500945526);
    EXPECT_TRUE(sample_source_0.has_stacks);
    const auto& sample_source_1 = data_provider.sample_sources()[1];
    EXPECT_EQ(sample_source_1.name, "cache-misses:u");
    EXPECT_EQ(sample_source_1.number_of_samples, 18128);
    EXPECT_EQ(sample_source_1.average_sampling_rate, 4690.0279109439043);
    EXPECT_TRUE(sample_source_1.has_stacks);
    const auto& sample_source_2 = data_provider.sample_sources()[2];
    EXPECT_EQ(sample_source_2.name, "cycles:u");
    EXPECT_EQ(sample_source_2.number_of_samples, 18716);
    EXPECT_EQ(sample_source_2.average_sampling_rate, 4842.2677087908423);
    EXPECT_TRUE(sample_source_2.has_stacks);
    const auto& sample_source_3 = data_provider.sample_sources()[3];
    EXPECT_EQ(sample_source_3.name, "branch-misses:u");
    EXPECT_EQ(sample_source_3.number_of_samples, 9692);
    EXPECT_EQ(sample_source_3.average_sampling_rate, 2569.6341831976838);
    EXPECT_TRUE(sample_source_3.has_stacks);

    const auto sampling_processes = to_vector(data_provider.sampling_processes());
    ASSERT_EQ(sampling_processes.size(), 1);
    const auto unique_sampling_process_id = sampling_processes[0];

    const auto process_info = data_provider.process_info(unique_sampling_process_id);
    EXPECT_EQ(process_info.unique_id, unique_sampling_process_id);
    EXPECT_EQ(process_info.os_id, 53);
    EXPECT_EQ(process_info.name, "ordered");
    EXPECT_EQ(process_info.start_time, 0ns);
    EXPECT_EQ(process_info.end_time, 3865225900ns);

    const auto threads = to_vector(data_provider.threads_info(unique_sampling_process_id));
    ASSERT_EQ(threads.size(), 1);

    EXPECT_EQ(threads[0].os_id, 53);
    EXPECT_EQ(threads[0].name, "ordered");
    EXPECT_EQ(threads[0].start_time, 0ns);
    EXPECT_EQ(threads[0].end_time, 3865225900ns);

    analysis::sample_filter filter;

    EXPECT_EQ(data_provider.count_samples(sample_source_0.id, unique_sampling_process_id, filter), 18603);
    EXPECT_EQ(data_provider.count_samples(sample_source_0.id, threads[0].unique_id, filter), 18603);

    EXPECT_EQ(data_provider.count_samples(sample_source_1.id, unique_sampling_process_id, filter), 18128);
    EXPECT_EQ(data_provider.count_samples(sample_source_1.id, threads[0].unique_id, filter), 18128);

    EXPECT_EQ(data_provider.count_samples(sample_source_2.id, unique_sampling_process_id, filter), 18716);
    EXPECT_EQ(data_provider.count_samples(sample_source_2.id, threads[0].unique_id, filter), 18716);

    EXPECT_EQ(data_provider.count_samples(sample_source_3.id, unique_sampling_process_id, filter), 9692);
    EXPECT_EQ(data_provider.count_samples(sample_source_3.id, threads[0].unique_id, filter), 9692);
}
