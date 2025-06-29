
#include <gtest/gtest.h>

#include <cmath>

#include <snail/analysis/analysis.hpp>
#include <snail/analysis/data_provider.hpp>

using namespace snail;
using namespace snail::analysis;

namespace {

struct test_sample_data : public sample_data
{
    test_sample_data(std::optional<stack_frame> frame, std::optional<std::vector<stack_frame>> frames) :
        frame_(std::move(frame)),
        frames_(std::move(frames))
    {}

    bool has_frame() const override
    {
        return frame_.has_value();
    }

    bool has_stack() const override
    {
        return frames_.has_value();
    }

    common::generator<stack_frame> reversed_stack() const override
    {
        if(!frames_) co_return;
        for(const auto& frame : *frames_)
        {
            co_yield stack_frame{frame};
        }
    }

    stack_frame frame() const override
    {
        if(!frame_) throw std::runtime_error("sample has no regular frame");
        return *frame_;
    }

    std::chrono::nanoseconds timestamp() const override
    {
        return std::chrono::nanoseconds(0); // not used in this test
    }

    std::optional<stack_frame>              frame_;
    std::optional<std::vector<stack_frame>> frames_;
};

class test_samples_provider : public samples_provider
{
public:
    const std::vector<sample_source_info>& sample_sources() const override
    {
        return sources_;
    }

    common::generator<const sample_data&> samples(sample_source_info::id_t source_id,
                                                  unique_process_id        process_id,
                                                  const sample_filter& /*filter*/) const override
    {
        if(process_id != expected_process_id_) co_return;
        if(!samples_.contains(source_id)) co_return;

        for(const auto& sample : samples_.at(source_id))
        {
            co_yield sample;
        }
    }
    std::size_t count_samples(sample_source_info::id_t source_id,
                              unique_process_id        process_id,
                              const sample_filter& /*filter*/) const override
    {
        if(process_id != expected_process_id_) return 0;
        if(!samples_.contains(source_id)) return 0;
        return samples_.at(source_id).size();
    }

    std::vector<sample_source_info> sources_;

    unique_process_id expected_process_id_;

    std::unordered_map<sample_source_info::id_t, std::vector<test_sample_data>> samples_;
};

using line_hits_map = std::unordered_map<std::size_t, source_hit_counts>;
using caller_map    = std::unordered_map<function_info::id_t, source_hit_counts>;

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

TEST(Analysis, SampleStacksFullInfo)
{
    const auto process_id = unique_process_id{.key = 123};

    const std::string function_a_name = "func_a";
    const std::string function_b_name = "func_b";
    const std::string function_c_name = "func_c";

    const std::string module_a_name = "mod_a.so";
    const std::string module_b_name = "mod_b.dll";

    const std::string file_a_path = "/home/path/to/file/a.cpp";
    const std::string file_b_path = "C:/path/to/file/b.h";

    test_samples_provider samples_provider;
    samples_provider.expected_process_id_ = process_id;
    samples_provider.sources_             = {
        {.id                    = 0,
         .name                  = "source A",
         .number_of_samples     = 0,
         .average_sampling_rate = 1.0,
         .has_stacks            = false},
        {.id                    = 1,
         .name                  = "source B",
         .number_of_samples     = 0,
         .average_sampling_rate = 1.0,
         .has_stacks            = true }
    };
    const auto source_id      = samples_provider.sources_[1].id;
    samples_provider.samples_ = {
        {1,
         {test_sample_data(
              std::nullopt,
              std::vector{stack_frame{
                              .symbol_name             = function_a_name,
                              .module_name             = module_a_name,
                              .file_path               = file_a_path,
                              .function_line_number    = 10,
                              .instruction_line_number = 15},
                          stack_frame{
                              .symbol_name             = function_b_name,
                              .module_name             = module_b_name,
                              .file_path               = file_b_path,
                              .function_line_number    = 100,
                              .instruction_line_number = 100}}),
          test_sample_data(
              std::nullopt,
              std::vector{stack_frame{
                              .symbol_name             = function_a_name,
                              .module_name             = module_a_name,
                              .file_path               = file_a_path,
                              .function_line_number    = 10,
                              .instruction_line_number = 15},
                          stack_frame{
                              .symbol_name             = function_c_name,
                              .module_name             = module_a_name,
                              .file_path               = file_a_path,
                              .function_line_number    = 50,
                              .instruction_line_number = 60},
                          stack_frame{
                              .symbol_name             = function_b_name,
                              .module_name             = module_b_name,
                              .file_path               = file_b_path,
                              .function_line_number    = 100,
                              .instruction_line_number = 110}}),
          test_sample_data(
              std::nullopt,
              std::vector<stack_frame>{})}}
    };

    const auto analysis_result = analyze_stacks(samples_provider, process_id);

    EXPECT_EQ(analysis_result.process_id, process_id);

    // Check that all functions are present
    EXPECT_EQ(analysis_result.all_functions().size(), 3);
    const auto func_a_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_a_name](const function_info& func)
                                                  { return func.name == function_a_name; });
    EXPECT_NE(func_a_iter, analysis_result.all_functions().end());

    const auto func_b_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_b_name](const function_info& func)
                                                  { return func.name == function_b_name; });
    EXPECT_NE(func_b_iter, analysis_result.all_functions().end());

    const auto func_c_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_c_name](const function_info& func)
                                                  { return func.name == function_c_name; });
    EXPECT_NE(func_c_iter, analysis_result.all_functions().end());

    // Check that all modules are present
    EXPECT_EQ(analysis_result.all_modules().size(), 2);
    const auto mod_a_iter = std::ranges::find_if(analysis_result.all_modules(), [&module_a_name](const module_info& mod)
                                                 { return mod.name == module_a_name; });
    EXPECT_NE(mod_a_iter, analysis_result.all_modules().end());
    const auto mod_b_iter = std::ranges::find_if(analysis_result.all_modules(), [&module_b_name](const module_info& mod)
                                                 { return mod.name == module_b_name; });
    EXPECT_NE(mod_b_iter, analysis_result.all_modules().end());

    // Check that all files are present
    EXPECT_EQ(analysis_result.all_files().size(), 2);
    const auto file_a_iter = std::ranges::find_if(analysis_result.all_files(), [&file_a_path](const file_info& file)
                                                  { return file.path == file_a_path; });
    EXPECT_NE(file_a_iter, analysis_result.all_files().end());
    const auto file_b_iter = std::ranges::find_if(analysis_result.all_files(), [&file_b_path](const file_info& file)
                                                  { return file.path == file_b_path; });
    EXPECT_NE(file_b_iter, analysis_result.all_files().end());

    // Check function root
    const auto& function_root = analysis_result.get_function_root();
    EXPECT_EQ(function_root.name, "root");

    // Check function ids
    EXPECT_EQ(&analysis_result.get_function(function_root.id), &function_root);
    EXPECT_EQ(&analysis_result.get_function(func_a_iter->id), &*func_a_iter);
    EXPECT_EQ(&analysis_result.get_function(func_b_iter->id), &*func_b_iter);
    EXPECT_EQ(&analysis_result.get_function(func_c_iter->id), &*func_c_iter);

    // Check module ids
    EXPECT_EQ(&analysis_result.get_module(mod_a_iter->id), &*mod_a_iter);
    EXPECT_EQ(&analysis_result.get_module(mod_b_iter->id), &*mod_b_iter);

    // Check file ids
    EXPECT_EQ(&analysis_result.get_file(file_a_iter->id), &*file_a_iter);
    EXPECT_EQ(&analysis_result.get_file(file_b_iter->id), &*file_b_iter);

    // Check function hits
    EXPECT_EQ(function_root.hits.get(source_id).total, 3);
    EXPECT_EQ(function_root.hits.get(source_id).self, 1);
    EXPECT_EQ(func_a_iter->hits.get(source_id).total, 2);
    EXPECT_EQ(func_a_iter->hits.get(source_id).self, 0);
    EXPECT_EQ(func_b_iter->hits.get(source_id).total, 2);
    EXPECT_EQ(func_b_iter->hits.get(source_id).self, 2);
    EXPECT_EQ(func_c_iter->hits.get(source_id).total, 1);
    EXPECT_EQ(func_c_iter->hits.get(source_id).self, 0);

    // Check module hits
    EXPECT_EQ(mod_a_iter->hits.get(source_id).total, 3);
    EXPECT_EQ(mod_a_iter->hits.get(source_id).self, 0);
    EXPECT_EQ(mod_b_iter->hits.get(source_id).total, 2);
    EXPECT_EQ(mod_b_iter->hits.get(source_id).self, 2);

    // Check file hits
    EXPECT_EQ(file_a_iter->hits.get(source_id).total, 3);
    EXPECT_EQ(file_a_iter->hits.get(source_id).self, 0);
    EXPECT_EQ(file_b_iter->hits.get(source_id).total, 2);
    EXPECT_EQ(file_b_iter->hits.get(source_id).self, 2);

    // Check function file associations
    EXPECT_EQ(func_a_iter->file_id, file_a_iter->id);
    EXPECT_EQ(func_b_iter->file_id, file_b_iter->id);
    EXPECT_EQ(func_c_iter->file_id, file_a_iter->id);

    // Check function module associations
    EXPECT_EQ(func_a_iter->module_id, mod_a_iter->id);
    EXPECT_EQ(func_b_iter->module_id, mod_b_iter->id);
    EXPECT_EQ(func_c_iter->module_id, mod_a_iter->id);

    // Check function line numbers
    EXPECT_EQ(func_a_iter->line_number, 10);
    EXPECT_EQ(func_b_iter->line_number, 100);
    EXPECT_EQ(func_c_iter->line_number, 50);

    // Check function line hits
    EXPECT_EQ(func_a_iter->hits_by_line,
              (line_hits_map{
                  {15, {{{}, {.total = 2, .self = 0}}}}
    }));
    EXPECT_EQ(func_b_iter->hits_by_line,
              (line_hits_map{
                  {100, {{{}, {.total = 1, .self = 1}}}},
                  {110, {{{}, {.total = 1, .self = 1}}}}
    }));
    EXPECT_EQ(func_c_iter->hits_by_line,
              (line_hits_map{
                  {60, {{{}, {.total = 1, .self = 0}}}}
    }));

    // Check function callers
    EXPECT_EQ(function_root.callers,
              (caller_map{}));
    EXPECT_EQ(func_a_iter->callers,
              (caller_map{
                  {function_root.id, {{{}, {.total = 2, .self = 0}}}}
    }));
    EXPECT_EQ(func_b_iter->callers,
              (caller_map{
                  {func_a_iter->id, {{{}, {.total = 1, .self = 0}}}},
                  {func_c_iter->id, {{{}, {.total = 1, .self = 0}}}}
    }));
    EXPECT_EQ(func_c_iter->callers,
              (caller_map{
                  {func_a_iter->id, {{{}, {.total = 1, .self = 0}}}}
    }));

    // Check function callees
    EXPECT_EQ(function_root.callees,
              (caller_map{
                  {func_a_iter->id, {{{}, {.total = 2, .self = 0}}}}
    }));
    EXPECT_EQ(func_a_iter->callees,
              (caller_map{
                  {func_b_iter->id, {{{}, {.total = 1, .self = 0}}}},
                  {func_c_iter->id, {{{}, {.total = 1, .self = 0}}}}
    }));
    EXPECT_EQ(func_b_iter->callees,
              (caller_map{}));
    EXPECT_EQ(func_c_iter->callees,
              (caller_map{
                  {func_b_iter->id, {{{}, {.total = 1, .self = 0}}}}
    }));

    // check call tree
    const auto& call_tree_root = analysis_result.get_call_tree_root();
    EXPECT_EQ(&analysis_result.get_call_tree_node(call_tree_root.id), &call_tree_root);
    EXPECT_EQ(call_tree_root.function_id, function_root.id);
    EXPECT_EQ(call_tree_root.hits, (source_hit_counts{
                                       {{}, {.total = 3, .self = 1}}
    }));
    EXPECT_EQ(call_tree_root.children.size(), 1);

    // root => func_a
    const auto call_tree_node_a = analysis_result.get_call_tree_node(call_tree_root.children[0]);
    EXPECT_EQ(call_tree_node_a.function_id, func_a_iter->id);
    EXPECT_EQ(call_tree_node_a.hits, (source_hit_counts{
                                         {{}, {.total = 2, .self = 0}}
    }));
    EXPECT_EQ(call_tree_node_a.children.size(), 2);

    // func_a => func_b
    const auto a_b_id_iter = std::ranges::find_if(call_tree_node_a.children, [&](const call_tree_node::id_t id)
                                                  { return analysis_result.get_call_tree_node(id).function_id == func_b_iter->id; });
    EXPECT_NE(a_b_id_iter, call_tree_node_a.children.end());
    const auto call_tree_node_a_b = analysis_result.get_call_tree_node(*a_b_id_iter);
    EXPECT_EQ(call_tree_node_a_b.id, *a_b_id_iter);
    EXPECT_EQ(call_tree_node_a_b.hits, (source_hit_counts{
                                           {{}, {.total = 1, .self = 1}}
    }));
    EXPECT_EQ(call_tree_node_a_b.children.size(), 0);

    // func_a => func_c
    const auto a_c_id_iter = std::ranges::find_if(call_tree_node_a.children, [&](const call_tree_node::id_t id)
                                                  { return analysis_result.get_call_tree_node(id).function_id == func_c_iter->id; });
    EXPECT_NE(a_c_id_iter, call_tree_node_a.children.end());
    const auto call_tree_node_a_c = analysis_result.get_call_tree_node(*a_c_id_iter);
    EXPECT_EQ(call_tree_node_a_c.id, *a_c_id_iter);
    EXPECT_EQ(call_tree_node_a_c.hits, (source_hit_counts{
                                           {{}, {.total = 1, .self = 0}}
    }));
    EXPECT_EQ(call_tree_node_a_c.children.size(), 1);

    // func_a => func_c => func_b
    const auto call_tree_node_a_c_b = analysis_result.get_call_tree_node(call_tree_node_a_c.children[0]);
    EXPECT_EQ(call_tree_node_a_c_b.function_id, func_b_iter->id);
    EXPECT_EQ(call_tree_node_a_c_b.hits, (source_hit_counts{
                                             {{}, {.total = 1, .self = 1}}
    }));
    EXPECT_EQ(call_tree_node_a_c_b.children.size(), 0);
}

TEST(Analysis, SampleStacksMissingFile)
{
    const auto process_id = unique_process_id{.key = 123};

    const std::string function_a_name = "func_a";
    const std::string function_b_name = "func_b";
    const std::string function_c_name = "func_c";
    const std::string function_d_name = "func_d";

    const std::string module_a_name = "mod_a.so";

    const std::string file_a_path = "/home/path/to/file/a.cpp";

    test_samples_provider samples_provider;
    samples_provider.expected_process_id_ = process_id;
    samples_provider.sources_             = {
        {.id                    = 0,
         .name                  = "source A",
         .number_of_samples     = 0,
         .average_sampling_rate = 1.0,
         .has_stacks            = false},
        {.id                    = 1,
         .name                  = "source B",
         .number_of_samples     = 0,
         .average_sampling_rate = 1.0,
         .has_stacks            = true }
    };
    const auto source_id      = samples_provider.sources_[1].id;
    samples_provider.samples_ = {
        {1,
         {test_sample_data(
              std::nullopt,
              std::vector{stack_frame{
                              .symbol_name             = function_a_name,
                              .module_name             = module_a_name,
                              .file_path               = file_a_path,
                              .function_line_number    = 10,
                              .instruction_line_number = 15},
                          stack_frame{
                              .symbol_name             = function_b_name,
                              .module_name             = module_a_name,
                              .file_path               = {},
                              .function_line_number    = {},
                              .instruction_line_number = {}}}),
          test_sample_data(
              std::nullopt,
              std::vector{stack_frame{
                              .symbol_name             = function_c_name,
                              .module_name             = module_a_name,
                              .file_path               = {},
                              .function_line_number    = {},
                              .instruction_line_number = {}},
                          stack_frame{
                              .symbol_name             = function_d_name,
                              .module_name             = module_a_name,
                              .file_path               = file_a_path,
                              .function_line_number    = 50,
                              .instruction_line_number = 60}})}}
    };

    const auto analysis_result = analyze_stacks(samples_provider, process_id);

    EXPECT_EQ(analysis_result.process_id, process_id);

    // Check that all functions are present
    EXPECT_EQ(analysis_result.all_functions().size(), 4);
    const auto func_a_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_a_name](const function_info& func)
                                                  { return func.name == function_a_name; });
    EXPECT_NE(func_a_iter, analysis_result.all_functions().end());

    const auto func_b_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_b_name](const function_info& func)
                                                  { return func.name == function_b_name; });
    EXPECT_NE(func_b_iter, analysis_result.all_functions().end());

    const auto func_c_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_c_name](const function_info& func)
                                                  { return func.name == function_c_name; });
    EXPECT_NE(func_c_iter, analysis_result.all_functions().end());

    const auto func_d_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_d_name](const function_info& func)
                                                  { return func.name == function_d_name; });
    EXPECT_NE(func_d_iter, analysis_result.all_functions().end());

    // Check that all modules are present
    EXPECT_EQ(analysis_result.all_modules().size(), 1);
    const auto mod_a_iter = std::ranges::find_if(analysis_result.all_modules(), [&module_a_name](const module_info& mod)
                                                 { return mod.name == module_a_name; });
    EXPECT_NE(mod_a_iter, analysis_result.all_modules().end());

    // Check that all files are present
    EXPECT_EQ(analysis_result.all_files().size(), 1);
    const auto file_a_iter = std::ranges::find_if(analysis_result.all_files(), [&file_a_path](const file_info& file)
                                                  { return file.path == file_a_path; });
    EXPECT_NE(file_a_iter, analysis_result.all_files().end());

    // Check function root
    const auto& function_root = analysis_result.get_function_root();
    EXPECT_EQ(function_root.name, "root");

    // Check function ids
    EXPECT_EQ(&analysis_result.get_function(function_root.id), &function_root);
    EXPECT_EQ(&analysis_result.get_function(func_a_iter->id), &*func_a_iter);
    EXPECT_EQ(&analysis_result.get_function(func_b_iter->id), &*func_b_iter);
    EXPECT_EQ(&analysis_result.get_function(func_c_iter->id), &*func_c_iter);
    EXPECT_EQ(&analysis_result.get_function(func_d_iter->id), &*func_d_iter);

    // Check module ids
    EXPECT_EQ(&analysis_result.get_module(mod_a_iter->id), &*mod_a_iter);

    // Check file ids
    EXPECT_EQ(&analysis_result.get_file(file_a_iter->id), &*file_a_iter);

    // Check function hits
    EXPECT_EQ(function_root.hits.get(source_id).total, 2);
    EXPECT_EQ(function_root.hits.get(source_id).self, 0);
    EXPECT_EQ(func_a_iter->hits.get(source_id).total, 1);
    EXPECT_EQ(func_a_iter->hits.get(source_id).self, 0);
    EXPECT_EQ(func_b_iter->hits.get(source_id).total, 1);
    EXPECT_EQ(func_b_iter->hits.get(source_id).self, 1);
    EXPECT_EQ(func_c_iter->hits.get(source_id).total, 1);
    EXPECT_EQ(func_c_iter->hits.get(source_id).self, 0);
    EXPECT_EQ(func_d_iter->hits.get(source_id).total, 1);
    EXPECT_EQ(func_d_iter->hits.get(source_id).self, 1);

    // Check module hits
    EXPECT_EQ(mod_a_iter->hits.get(source_id).total, 4);
    EXPECT_EQ(mod_a_iter->hits.get(source_id).self, 2);

    // Check file hits
    EXPECT_EQ(file_a_iter->hits.get(source_id).total, 2);
    EXPECT_EQ(file_a_iter->hits.get(source_id).self, 1);

    // Check function file associations
    EXPECT_EQ(func_a_iter->file_id, file_a_iter->id);
    EXPECT_EQ(func_b_iter->file_id, std::nullopt);
    EXPECT_EQ(func_c_iter->file_id, std::nullopt);
    EXPECT_EQ(func_d_iter->file_id, file_a_iter->id);

    // Check function module associations
    EXPECT_EQ(func_a_iter->module_id, mod_a_iter->id);
    EXPECT_EQ(func_b_iter->module_id, mod_a_iter->id);
    EXPECT_EQ(func_c_iter->module_id, mod_a_iter->id);
    EXPECT_EQ(func_d_iter->module_id, mod_a_iter->id);

    // Check function line numbers
    EXPECT_EQ(func_a_iter->line_number, 10);
    EXPECT_EQ(func_b_iter->line_number, std::nullopt);
    EXPECT_EQ(func_c_iter->line_number, std::nullopt);
    EXPECT_EQ(func_d_iter->line_number, 50);

    // Check function line hits
    EXPECT_EQ(func_a_iter->hits_by_line,
              (line_hits_map{
                  {15, {{{}, {.total = 1, .self = 0}}}}
    }));
    EXPECT_EQ(func_b_iter->hits_by_line,
              (line_hits_map{}));
    EXPECT_EQ(func_c_iter->hits_by_line,
              (line_hits_map{}));
    EXPECT_EQ(func_d_iter->hits_by_line,
              (line_hits_map{
                  {60, {{{}, {.total = 1, .self = 1}}}}
    }));

    // Check function callers
    EXPECT_EQ(function_root.callers,
              (caller_map{}));
    EXPECT_EQ(func_a_iter->callers,
              (caller_map{
                  {function_root.id, {{{}, {.total = 1, .self = 0}}}}
    }));
    EXPECT_EQ(func_b_iter->callers,
              (caller_map{
                  {func_a_iter->id, {{{}, {.total = 1, .self = 0}}}},
    }));
    EXPECT_EQ(func_c_iter->callers,
              (caller_map{
                  {function_root.id, {{{}, {.total = 1, .self = 0}}}}
    }));
    EXPECT_EQ(func_d_iter->callers,
              (caller_map{
                  {func_c_iter->id, {{{}, {.total = 1, .self = 0}}}},
    }));

    // Check function callees
    EXPECT_EQ(function_root.callees,
              (caller_map{
                  {func_a_iter->id, {{{}, {.total = 1, .self = 0}}}},
                  {func_c_iter->id, {{{}, {.total = 1, .self = 0}}}}
    }));
    EXPECT_EQ(func_a_iter->callees,
              (caller_map{
                  {func_b_iter->id, {{{}, {.total = 1, .self = 0}}}},
    }));
    EXPECT_EQ(func_b_iter->callees,
              (caller_map{}));
    EXPECT_EQ(func_c_iter->callees,
              (caller_map{
                  {func_d_iter->id, {{{}, {.total = 1, .self = 0}}}}
    }));
    EXPECT_EQ(func_d_iter->callees,
              (caller_map{}));

    // check call tree
    const auto& call_tree_root = analysis_result.get_call_tree_root();
    EXPECT_EQ(&analysis_result.get_call_tree_node(call_tree_root.id), &call_tree_root);
    EXPECT_EQ(call_tree_root.function_id, function_root.id);
    EXPECT_EQ(call_tree_root.hits, (source_hit_counts{
                                       {{}, {.total = 2, .self = 0}}
    }));
    EXPECT_EQ(call_tree_root.children.size(), 2);

    // root => func_a
    const auto a_id_iter = std::ranges::find_if(call_tree_root.children, [&](const call_tree_node::id_t id)
                                                { return analysis_result.get_call_tree_node(id).function_id == func_a_iter->id; });
    EXPECT_NE(a_id_iter, call_tree_root.children.end());
    const auto call_tree_node_a = analysis_result.get_call_tree_node(*a_id_iter);
    EXPECT_EQ(call_tree_node_a.id, *a_id_iter);
    EXPECT_EQ(call_tree_node_a.hits, (source_hit_counts{
                                         {{}, {.total = 1, .self = 0}}
    }));
    EXPECT_EQ(call_tree_node_a.children.size(), 1);

    // root => func_c
    const auto c_id_iter = std::ranges::find_if(call_tree_root.children, [&](const call_tree_node::id_t id)
                                                { return analysis_result.get_call_tree_node(id).function_id == func_c_iter->id; });
    EXPECT_NE(c_id_iter, call_tree_root.children.end());
    const auto call_tree_node_c = analysis_result.get_call_tree_node(*c_id_iter);
    EXPECT_EQ(call_tree_node_c.id, *c_id_iter);
    EXPECT_EQ(call_tree_node_c.hits, (source_hit_counts{
                                         {{}, {.total = 1, .self = 0}}
    }));
    EXPECT_EQ(call_tree_node_c.children.size(), 1);

    // root => func_a => func_b
    const auto call_tree_node_a_b = analysis_result.get_call_tree_node(call_tree_node_a.children[0]);
    EXPECT_EQ(call_tree_node_a_b.function_id, func_b_iter->id);
    EXPECT_EQ(call_tree_node_a_b.hits, (source_hit_counts{
                                           {{}, {.total = 1, .self = 1}}
    }));
    EXPECT_EQ(call_tree_node_a_b.children.size(), 0);

    // root => func_c => func_d
    const auto call_tree_node_c_d = analysis_result.get_call_tree_node(call_tree_node_c.children[0]);
    EXPECT_EQ(call_tree_node_c_d.function_id, func_d_iter->id);
    EXPECT_EQ(call_tree_node_c_d.hits, (source_hit_counts{
                                           {{}, {.total = 1, .self = 1}}
    }));
    EXPECT_EQ(call_tree_node_c_d.children.size(), 0);
}

TEST(Analysis, SampleNoStacks)
{
    const auto process_id = unique_process_id{.key = 123};

    const std::string function_a_name = "func_a";
    const std::string function_b_name = "func_b";
    const std::string function_c_name = "func_c";

    const std::string module_a_name = "mod_a.so";
    const std::string module_b_name = "mod_b.dll";

    const std::string file_a_path = "/home/path/to/file/a.cpp";
    const std::string file_b_path = "C:/path/to/file/b.h";

    test_samples_provider samples_provider;
    samples_provider.expected_process_id_ = process_id;

    samples_provider.sources_ = {
        {.id                    = 0,
         .name                  = "source A",
         .number_of_samples     = 0,
         .average_sampling_rate = 1.0,
         .has_stacks            = false},
        {.id                    = 1,
         .name                  = "source B",
         .number_of_samples     = 0,
         .average_sampling_rate = 1.0,
         .has_stacks            = true }
    };
    const auto source_id      = samples_provider.sources_[1].id;
    samples_provider.samples_ = {
        {1,
         {test_sample_data(
              stack_frame{
                  .symbol_name             = function_a_name,
                  .module_name             = module_a_name,
                  .file_path               = file_a_path,
                  .function_line_number    = 10,
                  .instruction_line_number = 15},
              std::nullopt),
          test_sample_data(
              stack_frame{
                  .symbol_name             = function_b_name,
                  .module_name             = module_b_name,
                  .file_path               = file_b_path,
                  .function_line_number    = 100,
                  .instruction_line_number = 100},
              std::nullopt),
          test_sample_data(
              stack_frame{
                  .symbol_name             = function_c_name,
                  .module_name             = module_a_name,
                  .file_path               = file_a_path,
                  .function_line_number    = 50,
                  .instruction_line_number = 60},
              std::nullopt),
          test_sample_data(
              stack_frame{
                  .symbol_name             = function_a_name,
                  .module_name             = module_a_name,
                  .file_path               = file_a_path,
                  .function_line_number    = 10,
                  .instruction_line_number = 12},
              std::nullopt),
          test_sample_data(
              std::nullopt,
              std::nullopt)}}
    };

    const auto analysis_result = analyze_stacks(samples_provider, process_id);

    EXPECT_EQ(analysis_result.process_id, process_id);

    // Check that all functions are present
    EXPECT_EQ(analysis_result.all_functions().size(), 3);
    const auto func_a_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_a_name](const function_info& func)
                                                  { return func.name == function_a_name; });
    EXPECT_NE(func_a_iter, analysis_result.all_functions().end());

    const auto func_b_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_b_name](const function_info& func)
                                                  { return func.name == function_b_name; });
    EXPECT_NE(func_b_iter, analysis_result.all_functions().end());

    const auto func_c_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_c_name](const function_info& func)
                                                  { return func.name == function_c_name; });
    EXPECT_NE(func_c_iter, analysis_result.all_functions().end());

    // Check that all modules are present
    EXPECT_EQ(analysis_result.all_modules().size(), 2);
    const auto mod_a_iter = std::ranges::find_if(analysis_result.all_modules(), [&module_a_name](const module_info& mod)
                                                 { return mod.name == module_a_name; });
    EXPECT_NE(mod_a_iter, analysis_result.all_modules().end());
    const auto mod_b_iter = std::ranges::find_if(analysis_result.all_modules(), [&module_b_name](const module_info& mod)
                                                 { return mod.name == module_b_name; });
    EXPECT_NE(mod_b_iter, analysis_result.all_modules().end());

    // Check that all files are present
    EXPECT_EQ(analysis_result.all_files().size(), 2);
    const auto file_a_iter = std::ranges::find_if(analysis_result.all_files(), [&file_a_path](const file_info& file)
                                                  { return file.path == file_a_path; });
    EXPECT_NE(file_a_iter, analysis_result.all_files().end());
    const auto file_b_iter = std::ranges::find_if(analysis_result.all_files(), [&file_b_path](const file_info& file)
                                                  { return file.path == file_b_path; });
    EXPECT_NE(file_b_iter, analysis_result.all_files().end());

    // Check function root
    const auto& function_root = analysis_result.get_function_root();
    EXPECT_EQ(function_root.name, "root");

    // Check function ids
    EXPECT_EQ(&analysis_result.get_function(function_root.id), &function_root);
    EXPECT_EQ(&analysis_result.get_function(func_a_iter->id), &*func_a_iter);
    EXPECT_EQ(&analysis_result.get_function(func_b_iter->id), &*func_b_iter);
    EXPECT_EQ(&analysis_result.get_function(func_c_iter->id), &*func_c_iter);

    // Check module ids
    EXPECT_EQ(&analysis_result.get_module(mod_a_iter->id), &*mod_a_iter);
    EXPECT_EQ(&analysis_result.get_module(mod_b_iter->id), &*mod_b_iter);

    // Check file ids
    EXPECT_EQ(&analysis_result.get_file(file_a_iter->id), &*file_a_iter);
    EXPECT_EQ(&analysis_result.get_file(file_b_iter->id), &*file_b_iter);

    // Check function hits
    EXPECT_EQ(function_root.hits.get(source_id).total, 0);
    EXPECT_EQ(function_root.hits.get(source_id).self, 0);
    EXPECT_EQ(func_a_iter->hits.get(source_id).total, 2);
    EXPECT_EQ(func_a_iter->hits.get(source_id).self, 2);
    EXPECT_EQ(func_b_iter->hits.get(source_id).total, 1);
    EXPECT_EQ(func_b_iter->hits.get(source_id).self, 1);
    EXPECT_EQ(func_c_iter->hits.get(source_id).total, 1);
    EXPECT_EQ(func_c_iter->hits.get(source_id).self, 1);

    // Check module hits
    EXPECT_EQ(mod_a_iter->hits.get(source_id).total, 3);
    EXPECT_EQ(mod_a_iter->hits.get(source_id).self, 3);
    EXPECT_EQ(mod_b_iter->hits.get(source_id).total, 1);
    EXPECT_EQ(mod_b_iter->hits.get(source_id).self, 1);

    // Check file hits
    EXPECT_EQ(file_a_iter->hits.get(source_id).total, 3);
    EXPECT_EQ(file_a_iter->hits.get(source_id).self, 3);
    EXPECT_EQ(file_b_iter->hits.get(source_id).total, 1);
    EXPECT_EQ(file_b_iter->hits.get(source_id).self, 1);

    // Check function file associations
    EXPECT_EQ(func_a_iter->file_id, file_a_iter->id);
    EXPECT_EQ(func_b_iter->file_id, file_b_iter->id);
    EXPECT_EQ(func_c_iter->file_id, file_a_iter->id);

    // Check function module associations
    EXPECT_EQ(func_a_iter->module_id, mod_a_iter->id);
    EXPECT_EQ(func_b_iter->module_id, mod_b_iter->id);
    EXPECT_EQ(func_c_iter->module_id, mod_a_iter->id);

    // Check function line numbers
    EXPECT_EQ(func_a_iter->line_number, 10);
    EXPECT_EQ(func_b_iter->line_number, 100);
    EXPECT_EQ(func_c_iter->line_number, 50);

    // Check function line hits
    EXPECT_EQ(func_a_iter->hits_by_line,
              (line_hits_map{
                  {12, {{{}, {.total = 1, .self = 1}}}},
                  {15, {{{}, {.total = 1, .self = 1}}}},
    }));
    EXPECT_EQ(func_b_iter->hits_by_line,
              (line_hits_map{
                  {100, {{{}, {.total = 1, .self = 1}}}},
    }));
    EXPECT_EQ(func_c_iter->hits_by_line,
              (line_hits_map{
                  {60, {{{}, {.total = 1, .self = 1}}}}
    }));

    // Check function callers
    EXPECT_EQ(function_root.callers,
              (caller_map{}));
    EXPECT_EQ(func_a_iter->callers,
              (caller_map{}));
    EXPECT_EQ(func_b_iter->callers,
              (caller_map{}));
    EXPECT_EQ(func_c_iter->callers,
              (caller_map{}));

    // Check function callees
    EXPECT_EQ(function_root.callees,
              (caller_map{}));
    EXPECT_EQ(func_a_iter->callees,
              (caller_map{}));
    EXPECT_EQ(func_b_iter->callees,
              (caller_map{}));
    EXPECT_EQ(func_c_iter->callees,
              (caller_map{}));

    // check call tree
    const auto& call_tree_root = analysis_result.get_call_tree_root();
    EXPECT_EQ(&analysis_result.get_call_tree_node(call_tree_root.id), &call_tree_root);
    EXPECT_EQ(call_tree_root.function_id, function_root.id);
    EXPECT_EQ(call_tree_root.hits, (source_hit_counts{
                                       {{}, {.total = 0, .self = 0}}
    }));
    EXPECT_EQ(call_tree_root.children.size(), 0);
}

TEST(Analysis, SampleNoStacksCancel)
{
    const auto process_id = unique_process_id{.key = 123};

    const std::string function_a_name = "func_a";
    const std::string function_b_name = "func_b";
    const std::string function_c_name = "func_c";

    const std::string module_a_name = "mod_a.so";
    const std::string module_b_name = "mod_b.dll";

    const std::string file_a_path = "/home/path/to/file/a.cpp";
    const std::string file_b_path = "C:/path/to/file/b.h";

    test_samples_provider samples_provider;
    samples_provider.expected_process_id_ = process_id;

    samples_provider.sources_ = {
        {.id                    = 0,
         .name                  = "source A",
         .number_of_samples     = 0,
         .average_sampling_rate = 1.0,
         .has_stacks            = false},
        {.id                    = 1,
         .name                  = "source B",
         .number_of_samples     = 0,
         .average_sampling_rate = 1.0,
         .has_stacks            = true }
    };
    const auto source_id      = samples_provider.sources_[1].id;
    samples_provider.samples_ = {
        {1,
         {test_sample_data(
              stack_frame{
                  .symbol_name             = function_a_name,
                  .module_name             = module_a_name,
                  .file_path               = file_a_path,
                  .function_line_number    = 10,
                  .instruction_line_number = 15},
              std::nullopt),
          test_sample_data(
              stack_frame{
                  .symbol_name             = function_b_name,
                  .module_name             = module_b_name,
                  .file_path               = file_b_path,
                  .function_line_number    = 100,
                  .instruction_line_number = 100},
              std::nullopt),
          test_sample_data(
              stack_frame{
                  .symbol_name             = function_c_name,
                  .module_name             = module_a_name,
                  .file_path               = file_a_path,
                  .function_line_number    = 50,
                  .instruction_line_number = 60},
              std::nullopt),
          test_sample_data(
              stack_frame{
                  .symbol_name             = function_a_name,
                  .module_name             = module_a_name,
                  .file_path               = file_a_path,
                  .function_line_number    = 10,
                  .instruction_line_number = 12},
              std::nullopt),
          test_sample_data(
              std::nullopt,
              std::nullopt)}}
    };

    {
        // Do not cancel at all
        const test_progress_listener progress_listener(0.1);

        const auto analysis_result = analyze_stacks(samples_provider, process_id, {}, &progress_listener);

        EXPECT_TRUE(progress_listener.started);
        EXPECT_TRUE(progress_listener.finished);
        EXPECT_EQ(progress_listener.reported,
                  (std::vector<double>{0.0, 20.0, 40.0, 60.0, 80.0, 100.0}));

        // Do need to test this. This is the same as the test above
    }
    {
        // Cancel at 40%
        test_progress_listener progress_listener(0.1);
        progress_listener.cancel_at = 0.4;

        const auto analysis_result = analyze_stacks(samples_provider, process_id, {}, &progress_listener, &progress_listener.token);

        EXPECT_TRUE(progress_listener.started);
        EXPECT_FALSE(progress_listener.finished);
        EXPECT_EQ(progress_listener.reported,
                  (std::vector<double>{0.0, 20.0, 40.0}));

        EXPECT_EQ(analysis_result.process_id, process_id);

        // Check that all functions are present
        EXPECT_EQ(analysis_result.all_functions().size(), 2);
        const auto func_a_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_a_name](const function_info& func)
                                                      { return func.name == function_a_name; });
        EXPECT_NE(func_a_iter, analysis_result.all_functions().end());

        const auto func_b_iter = std::ranges::find_if(analysis_result.all_functions(), [&function_b_name](const function_info& func)
                                                      { return func.name == function_b_name; });
        EXPECT_NE(func_b_iter, analysis_result.all_functions().end());

        // Check that all modules are present
        EXPECT_EQ(analysis_result.all_modules().size(), 2);
        const auto mod_a_iter = std::ranges::find_if(analysis_result.all_modules(), [&module_a_name](const module_info& mod)
                                                     { return mod.name == module_a_name; });
        EXPECT_NE(mod_a_iter, analysis_result.all_modules().end());
        const auto mod_b_iter = std::ranges::find_if(analysis_result.all_modules(), [&module_b_name](const module_info& mod)
                                                     { return mod.name == module_b_name; });
        EXPECT_NE(mod_b_iter, analysis_result.all_modules().end());

        // Check that all files are present
        EXPECT_EQ(analysis_result.all_files().size(), 2);
        const auto file_a_iter = std::ranges::find_if(analysis_result.all_files(), [&file_a_path](const file_info& file)
                                                      { return file.path == file_a_path; });
        EXPECT_NE(file_a_iter, analysis_result.all_files().end());
        const auto file_b_iter = std::ranges::find_if(analysis_result.all_files(), [&file_b_path](const file_info& file)
                                                      { return file.path == file_b_path; });
        EXPECT_NE(file_b_iter, analysis_result.all_files().end());

        // Check function root
        const auto& function_root = analysis_result.get_function_root();
        EXPECT_EQ(function_root.name, "root");

        // Check function ids
        EXPECT_EQ(&analysis_result.get_function(function_root.id), &function_root);
        EXPECT_EQ(&analysis_result.get_function(func_a_iter->id), &*func_a_iter);
        EXPECT_EQ(&analysis_result.get_function(func_b_iter->id), &*func_b_iter);

        // Check module ids
        EXPECT_EQ(&analysis_result.get_module(mod_a_iter->id), &*mod_a_iter);
        EXPECT_EQ(&analysis_result.get_module(mod_b_iter->id), &*mod_b_iter);

        // Check file ids
        EXPECT_EQ(&analysis_result.get_file(file_a_iter->id), &*file_a_iter);
        EXPECT_EQ(&analysis_result.get_file(file_b_iter->id), &*file_b_iter);

        // Check function hits
        EXPECT_EQ(function_root.hits.get(source_id).total, 0);
        EXPECT_EQ(function_root.hits.get(source_id).self, 0);
        EXPECT_EQ(func_a_iter->hits.get(source_id).total, 1);
        EXPECT_EQ(func_a_iter->hits.get(source_id).self, 1);
        EXPECT_EQ(func_b_iter->hits.get(source_id).total, 1);
        EXPECT_EQ(func_b_iter->hits.get(source_id).self, 1);

        // Check module hits
        EXPECT_EQ(mod_a_iter->hits.get(source_id).total, 1);
        EXPECT_EQ(mod_a_iter->hits.get(source_id).self, 1);
        EXPECT_EQ(mod_b_iter->hits.get(source_id).total, 1);
        EXPECT_EQ(mod_b_iter->hits.get(source_id).self, 1);

        // Check file hits
        EXPECT_EQ(file_a_iter->hits.get(source_id).total, 1);
        EXPECT_EQ(file_a_iter->hits.get(source_id).self, 1);
        EXPECT_EQ(file_b_iter->hits.get(source_id).total, 1);
        EXPECT_EQ(file_b_iter->hits.get(source_id).self, 1);

        // Check function file associations
        EXPECT_EQ(func_a_iter->file_id, file_a_iter->id);
        EXPECT_EQ(func_b_iter->file_id, file_b_iter->id);

        // Check function module associations
        EXPECT_EQ(func_a_iter->module_id, mod_a_iter->id);
        EXPECT_EQ(func_b_iter->module_id, mod_b_iter->id);

        // Check function line numbers
        EXPECT_EQ(func_a_iter->line_number, 10);
        EXPECT_EQ(func_b_iter->line_number, 100);

        // Check function line hits
        EXPECT_EQ(func_a_iter->hits_by_line,
                  (line_hits_map{
                      {15, {{{}, {.total = 1, .self = 1}}}},
        }));
        EXPECT_EQ(func_b_iter->hits_by_line,
                  (line_hits_map{
                      {100, {{{}, {.total = 1, .self = 1}}}},
        }));

        // Check function callers
        EXPECT_EQ(function_root.callers,
                  (caller_map{}));
        EXPECT_EQ(func_a_iter->callers,
                  (caller_map{}));
        EXPECT_EQ(func_b_iter->callers,
                  (caller_map{}));

        // Check function callees
        EXPECT_EQ(function_root.callees,
                  (caller_map{}));
        EXPECT_EQ(func_a_iter->callees,
                  (caller_map{}));
        EXPECT_EQ(func_b_iter->callees,
                  (caller_map{}));

        // check call tree
        const auto& call_tree_root = analysis_result.get_call_tree_root();
        EXPECT_EQ(&analysis_result.get_call_tree_node(call_tree_root.id), &call_tree_root);
        EXPECT_EQ(call_tree_root.function_id, function_root.id);
        EXPECT_EQ(call_tree_root.hits, (source_hit_counts{
                                           {{}, {.total = 0, .self = 0}}
        }));
        EXPECT_EQ(call_tree_root.children.size(), 0);
    }

    {
        // Cancel right away
        test_progress_listener progress_listener(0.1);
        progress_listener.token.cancel();

        const auto analysis_result = analyze_stacks(samples_provider, process_id, {}, &progress_listener, &progress_listener.token);

        EXPECT_TRUE(progress_listener.started);
        EXPECT_FALSE(progress_listener.finished);
        EXPECT_EQ(progress_listener.reported,
                  (std::vector<double>{0.0}));

        EXPECT_EQ(analysis_result.process_id, process_id);

        // Check that there are no function, modules and files yet.
        EXPECT_EQ(analysis_result.all_functions().size(), 0);
        EXPECT_EQ(analysis_result.all_modules().size(), 0);
        EXPECT_EQ(analysis_result.all_files().size(), 0);

        // Check for a valid function root
        const auto& function_root = analysis_result.get_function_root();
        EXPECT_EQ(function_root.name, "root");

        EXPECT_EQ(&analysis_result.get_function(function_root.id), &function_root);

        EXPECT_EQ(function_root.hits.get(source_id).total, 0);
        EXPECT_EQ(function_root.hits.get(source_id).self, 0);

        EXPECT_EQ(function_root.callers,
                  (caller_map{}));
        EXPECT_EQ(function_root.callees,
                  (caller_map{}));

        const auto& call_tree_root = analysis_result.get_call_tree_root();
        EXPECT_EQ(&analysis_result.get_call_tree_node(call_tree_root.id), &call_tree_root);
        EXPECT_EQ(call_tree_root.function_id, function_root.id);
        EXPECT_EQ(call_tree_root.hits, (source_hit_counts{
                                           {{}, {.total = 0, .self = 0}}
        }));
        EXPECT_EQ(call_tree_root.children.size(), 0);
    }
}
