#pragma once

#include <filesystem>
#include <memory>
#include <span>
#include <variant>

#include <snail/analysis/data/functions.hpp>
#include <snail/analysis/data/sample_source.hpp>

namespace snail::analysis {

struct stacks_analysis;
struct options;
struct sample_filter;
struct unique_process_id;
class path_map;
class data_provider;

} // namespace snail::analysis

namespace snail::server {

struct document_id
{
    std::size_t id_;
};

struct sort_by_name
{};

struct sort_by_samples
{
    enum class sum_type
    {
        total,
        self
    };

    analysis::sample_source_info::id_t sample_source_id;
    sum_type                           sum;
};

using sort_by_kind = std::variant<sort_by_name, sort_by_samples>;

class storage
{
public:
    storage();
    ~storage();

    analysis::options& get_options();

    analysis::path_map& get_module_path_map();

    document_id read_document(const std::filesystem::path& path);
    void        close_document(const document_id& id);

    const analysis::data_provider& get_data(const document_id& id);

    void apply_document_filter(const document_id& id, analysis::sample_filter filter);

    const std::unordered_map<analysis::sample_source_info::id_t, std::size_t>& get_total_samples_counts(const document_id& id);

    const analysis::stacks_analysis& get_stacks_analysis(const document_id& id, analysis::unique_process_id process_id);

    std::span<const analysis::function_info::id_t> get_functions_page(const document_id&          id,
                                                                      analysis::unique_process_id process_id,
                                                                      sort_by_kind                sort_by,
                                                                      bool                        reversed,
                                                                      std::size_t                 page_size,
                                                                      std::size_t                 page_index);

private:
    struct impl;

    std::unique_ptr<impl> impl_;
};

} // namespace snail::server
