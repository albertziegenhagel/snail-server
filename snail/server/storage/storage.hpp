#pragma once

#include <filesystem>
#include <memory>
#include <span>

#include <snail/common/types.hpp>

#include <snail/analysis/data/functions.hpp>

namespace snail::analysis {

struct stacks_analysis;
class data_provider;

} // namespace snail::analysis

namespace snail::server {

struct document_id
{
    std::size_t id_;
};

enum class function_data_type
{
    name,
    self_samples,
    total_samples,
};

enum class direction
{
    ascending,
    descending
};

class storage
{
public:
    storage();
    ~storage();

    document_id read_document(const std::filesystem::path& path);
    void        close_document(const document_id& id);

    const analysis::data_provider& get_data(const document_id& id);

    const analysis::stacks_analysis& get_stacks_analysis(const document_id& id, common::process_id_t process_id);

    std::span<const analysis::function_info::id_t> get_functions_page(const document_id& id, common::process_id_t process_id,
                                                                      function_data_type sort_by, bool reversed,
                                                                      std::size_t page_size, std::size_t page_index);

private:
    struct impl;

    std::unique_ptr<impl> impl_;
};

} // namespace snail::server
