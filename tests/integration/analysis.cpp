
#include <gtest/gtest.h>

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

TEST(DiagsessionDataProvider, Process)
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

    data_provider.process(file_path);

    EXPECT_EQ(data_provider.session_info().command_line, "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\Team Tools\\DiagnosticsHub\\Collector\\VSDiagnostics.exe\" start 8822D5E9-64DD-5269-B4F5-5387BD6C2FCB /launch:D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe /loadAgent:4EA90761-2248-496C-B854-3C0399A591A4;DiagnosticsHub.CpuAgent.dll ");
    EXPECT_EQ(data_provider.session_info().date, std::chrono::sys_seconds(1688212877s));
    EXPECT_EQ(data_provider.session_info().runtime, 3465175800ns);
    EXPECT_EQ(data_provider.session_info().number_of_processes, 1);
    EXPECT_EQ(data_provider.session_info().number_of_threads, 4);
    EXPECT_EQ(data_provider.session_info().number_of_samples, 292);
    EXPECT_EQ(data_provider.session_info().average_sampling_rate, 84.267008906157088);

    EXPECT_EQ(data_provider.system_info().hostname, "fv-az448-164");
    EXPECT_EQ(data_provider.system_info().platform, "Windows Server 2022 Datacenter");
    EXPECT_EQ(data_provider.system_info().architecture, "x64");
    EXPECT_EQ(data_provider.system_info().cpu_name, "Intel(R) Xeon(R) Platinum 8272CL CPU @ 2.60GHz");
    EXPECT_EQ(data_provider.system_info().number_of_processors, 2);

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
          {"mainCRTStartup", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_main.cpp", 14, 16},
          {"__scrt_common_main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 323, 330},
          {"__scrt_common_main_seh", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 235, 287},
          {"invoke_main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 76, 78},
          {"main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/snail-server/snail-server/tests/apps/inner/main.cpp", 57, 69},
          {"void __cdecl make_random_vector(class std::vector<double, class std::allocator<double>> &, unsigned __int64)", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/snail-server/snail-server/tests/apps/inner/main.cpp", 11, 19}}                                                          },
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
          {"mainCRTStartup", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_main.cpp", 14, 16},
          {"__scrt_common_main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 323, 330},
          {"__scrt_common_main_seh", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 235, 287},
          {"invoke_main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/_work/1/s/src/vctools/crt/vcstartup/src/startup/exe_common.inl", 76, 78},
          {"main", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/snail-server/snail-server/tests/apps/inner/main.cpp", 57, 71},
          {"double __cdecl compute_inner_product(class std::vector<double, class std::allocator<double>> const &, class std::vector<double, class std::allocator<double>> const &)", "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe", "D:/a/snail-server/snail-server/tests/apps/inner/main.cpp", 27, 36}}}
    };

    // No filter
    sample_count   = 0;
    last_timestamp = 0ns;
    for(const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 292);

    // Filter complete range
    filter.min_time = 1073706700ns;
    filter.max_time = 2563852500ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 292);

    // Filter first half
    filter.min_time = std::nullopt;
    filter.max_time = 1818779600ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 11);

    // Filter second half
    filter.min_time = 1818779600ns;
    filter.max_time = std::nullopt;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 281);

    // Filter somewhere in the middle
    filter.min_time = 2200000000ns;
    filter.max_time = 2300000000ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 99);

    // None empty
    filter.min_time = 2300000000ns;
    filter.max_time = 2200000000ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
    {
        ++sample_count;
    }
    EXPECT_EQ(sample_count, 0);
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 0);

    // Filter one unimportant thread
    filter.min_time = std::nullopt;
    filter.max_time = std::nullopt;
    filter.excluded_threads.clear();
    filter.excluded_threads.insert(thread_3148.unique_id);
    sample_count   = 0;
    last_timestamp = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 291);

    // Filter the main thread and another one
    filter.min_time = std::nullopt;
    filter.max_time = std::nullopt;
    filter.excluded_threads.clear();
    filter.excluded_threads.insert(thread_3828.unique_id);
    filter.excluded_threads.insert(thread_4224.unique_id);
    sample_count   = 0;
    last_timestamp = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
    {
        ++sample_count;

        const auto timestamp = sample.timestamp();
        ASSERT_LE(last_timestamp, timestamp);
        last_timestamp = timestamp;
    }
    EXPECT_EQ(sample_count, 3);
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 3);

    // Filter the process
    filter.min_time = std::nullopt;
    filter.max_time = std::nullopt;
    filter.excluded_threads.clear();
    filter.excluded_processes.insert(unique_sampling_process_id);
    sample_count   = 0;
    last_timestamp = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
    {
        ++sample_count;
    }
    EXPECT_EQ(sample_count, 0);
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 0);
}

TEST(PerfDataDataProvider, Process)
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

    data_provider.process(file_path);

    EXPECT_EQ(data_provider.session_info().command_line, "/usr/bin/perf record -g -o inner-perf.data /tmp/build/inner/Debug/build/inner");
    // EXPECT_EQ(data_provider.session_info().date, std::chrono::sys_seconds(1688327245s)); // This is not stable
    EXPECT_EQ(data_provider.session_info().runtime, 387398400ns);
    EXPECT_EQ(data_provider.session_info().number_of_processes, 1);
    EXPECT_EQ(data_provider.session_info().number_of_threads, 1);
    EXPECT_EQ(data_provider.session_info().number_of_samples, 1524);
    EXPECT_EQ(data_provider.session_info().average_sampling_rate, 3933.9346780988258);

    EXPECT_EQ(data_provider.system_info().hostname, "DESKTOP-0RH3TEF");
    EXPECT_EQ(data_provider.system_info().platform, "5.15.90.1-microsoft-standard-WSL2");
    EXPECT_EQ(data_provider.system_info().architecture, "x86_64");
    EXPECT_EQ(data_provider.system_info().cpu_name, "Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz");
    EXPECT_EQ(data_provider.system_info().number_of_processors, 8);

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
          {"main", "/tmp/build/inner/Debug/build/inner", "/tmp/snail-server/tests/apps/inner/main.cpp", 57, 69},
          {"make_random_vector(std::vector<double, std::allocator<double>>&, unsigned long)", "/tmp/build/inner/Debug/build/inner", "/tmp/snail-server/tests/apps/inner/main.cpp", 11, 21}}                                              },
        {1502,
         {{"inner!0x0000000000402255", "/tmp/build/inner/Debug/build/inner", "", 0, 0},
          {"libc.so.6!0x00007f6062938c0b", "/usr/lib64/libc.so.6", "", 0, 0},
          {"libc.so.6!0x00007f6062938b4a", "/usr/lib64/libc.so.6", "", 0, 0},
          {"main", "/tmp/build/inner/Debug/build/inner", "/tmp/snail-server/tests/apps/inner/main.cpp", 57, 71},
          {"compute_inner_product(std::vector<double, std::allocator<double>> const&, std::vector<double, std::allocator<double>> const&)", "/tmp/build/inner/Debug/build/inner", "/tmp/snail-server/tests/apps/inner/main.cpp", 26, 35}}}
    };

    // No filter
    sample_count   = 0;
    last_timestamp = 0ns;
    for(const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 1524);

    // Filter complete range
    filter.min_time = 0ns;
    filter.max_time = 387398400ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 1524);

    // Filter first half
    filter.min_time = std::nullopt;
    filter.max_time = 193699200ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 755);

    // Filter second half
    filter.min_time = 193699200ns;
    filter.max_time = std::nullopt;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 769);

    // Filter somewhere in the middle
    filter.min_time = 180000000ns;
    filter.max_time = 200000000ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for(const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
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
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 80);

    // None empty
    filter.min_time = 230000000ns;
    filter.max_time = 220000000ns;
    sample_count    = 0;
    last_timestamp  = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
    {
        ++sample_count;
    }
    EXPECT_EQ(sample_count, 0);
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 0);

    // Filter the only thread
    filter.min_time = std::nullopt;
    filter.max_time = std::nullopt;
    filter.excluded_threads.clear();
    filter.excluded_threads.insert(threads[0].unique_id);
    sample_count   = 0;
    last_timestamp = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
    {
        ++sample_count;
    }
    EXPECT_EQ(sample_count, 0);
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 0);

    // Filter the process
    filter.min_time = std::nullopt;
    filter.max_time = std::nullopt;
    filter.excluded_threads.clear();
    filter.excluded_processes.insert(unique_sampling_process_id);
    sample_count   = 0;
    last_timestamp = 0ns;
    for([[maybe_unused]] const auto& sample : data_provider.samples(unique_sampling_process_id, filter))
    {
        ++sample_count;
    }
    EXPECT_EQ(sample_count, 0);
    EXPECT_EQ(data_provider.count_samples(unique_sampling_process_id, filter), 0);
}
