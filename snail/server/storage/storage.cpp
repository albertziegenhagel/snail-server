
#include <snail/server/storage/storage.hpp>

#include <algorithm>
#include <ranges>
#include <unordered_map>

#include <snail/analysis/analysis.hpp>
#include <snail/analysis/data_provider.hpp>
#include <snail/analysis/options.hpp>
#include <snail/analysis/path_map.hpp>

using namespace snail;
using namespace snail::server;

namespace {

struct document_storage
{
    struct analysis_data
    {
        std::optional<analysis::stacks_analysis> stacks_analysis;

        std::optional<std::vector<analysis::function_info::id_t>> functions_by_self_samples;
        std::optional<std::vector<analysis::function_info::id_t>> functions_by_total_samples;
        std::optional<std::vector<analysis::function_info::id_t>> functions_by_name;
    };

    std::filesystem::path                    path;
    std::unique_ptr<analysis::data_provider> data_provider;
    analysis::sample_filter                  filter;

    std::optional<std::size_t> total_samples_count;

    std::unordered_map<analysis::unique_process_id, analysis_data> analysis_per_process;

    std::size_t get_total_samples_count()
    {
        if(total_samples_count) return *total_samples_count;

        total_samples_count = 0;

        for(const auto process_id : data_provider->sampling_processes())
        {
            (*total_samples_count) += data_provider->count_samples(process_id, filter);
        }

        return *total_samples_count;
    }

    const analysis::stacks_analysis& get_process_analysis(analysis::unique_process_id process_id)
    {
        auto& data = analysis_per_process[process_id];

        if(data.stacks_analysis == std::nullopt)
        {
            data = analysis_data{
                .stacks_analysis            = snail::analysis::analyze_stacks(*data_provider, process_id, filter),
                .functions_by_self_samples  = {},
                .functions_by_total_samples = {},
                .functions_by_name          = {},
            };
        }
        return *data.stacks_analysis;
    }

    const std::vector<analysis::function_info::id_t>& get_sorted_functions(analysis::unique_process_id process_id,
                                                                           function_data_type          sort_by)
    {
        const auto& stacks_analysis = get_process_analysis(process_id);

        const auto function_ids_view = stacks_analysis.all_functions() |
                                       std::views::transform([](const snail::analysis::function_info& function)
                                                             { return function.id; });

        auto function_ids = std::vector<snail::analysis::function_info::id_t>(function_ids_view.begin(), function_ids_view.end());

        auto& data = analysis_per_process[process_id];
        switch(sort_by)
        {
        case function_data_type::name:
            if(data.functions_by_name == std::nullopt)
            {
                data.functions_by_name = std::move(function_ids);
                std::ranges::sort(*data.functions_by_name, [&stacks_analysis](const snail::analysis::function_info::id_t& lhs, const snail::analysis::function_info::id_t& rhs)
                                  { return stacks_analysis.get_function(lhs).name < stacks_analysis.get_function(rhs).name; });
            }
            return *data.functions_by_self_samples;
        case function_data_type::self_samples:
            if(data.functions_by_self_samples == std::nullopt)
            {
                data.functions_by_self_samples = std::move(function_ids);
                std::ranges::sort(*data.functions_by_self_samples, [&stacks_analysis](const snail::analysis::function_info::id_t& lhs, const snail::analysis::function_info::id_t& rhs)
                                  { return stacks_analysis.get_function(lhs).hits.self < stacks_analysis.get_function(rhs).hits.self; });
            }
            return *data.functions_by_self_samples;
        case function_data_type::total_samples:
            if(data.functions_by_total_samples == std::nullopt)
            {
                data.functions_by_total_samples = std::move(function_ids);
                std::ranges::sort(*data.functions_by_total_samples, [&stacks_analysis](const snail::analysis::function_info::id_t& lhs, const snail::analysis::function_info::id_t& rhs)
                                  { return stacks_analysis.get_function(lhs).hits.total < stacks_analysis.get_function(rhs).hits.total; });
            }
            return *data.functions_by_total_samples;
        }

        throw std::runtime_error("Invalid function sort-by value");
    }
};

} // namespace

struct storage::impl
{
    analysis::options                                 options;
    analysis::path_map                                module_path_map;
    std::unordered_map<std::size_t, document_storage> open_documents;

    document_storage& get_document_storage(const document_id& id)
    {
        auto iter = open_documents.find(id.id_);
        if(iter == open_documents.end()) throw std::runtime_error("invalid document id");

        return iter->second;
    }

    document_id take_document_id()
    {
        return document_id{next_document_id_++};
    }

private:
    std::size_t next_document_id_ = 0;
};

storage::storage() :
    impl_(std::make_unique<impl>())
{}

storage::~storage() = default;

analysis::options& storage::get_options()
{
    return impl_->options;
}

analysis::path_map& storage::get_module_path_map()
{
    return impl_->module_path_map;
}

document_id storage::read_document(const std::filesystem::path& path)
{
    auto data_provider = analysis::make_data_provider(path.extension(),
                                                      impl_->options,
                                                      impl_->module_path_map);

    data_provider->process(path);

    const auto new_id                 = impl_->take_document_id();
    impl_->open_documents[new_id.id_] = document_storage{
        .path                 = path,
        .data_provider        = std::move(data_provider),
        .filter               = {},
        .total_samples_count  = {},
        .analysis_per_process = {},
    };

    return new_id;
}

void storage::close_document(const document_id& id)
{
    auto iter = impl_->open_documents.find(id.id_);
    if(iter == impl_->open_documents.end()) return;

    impl_->open_documents.erase(iter);
}

const analysis::data_provider& storage::get_data(const server::document_id& document_id)
{
    auto& document = impl_->get_document_storage(document_id);
    return *document.data_provider;
}

void storage::apply_document_filter(const document_id& document_id, analysis::sample_filter filter)
{
    auto& document = impl_->get_document_storage(document_id);
    if(document.filter == filter) return;
    document.filter = std::move(filter);
    document.total_samples_count.reset();
    document.analysis_per_process.clear();
}

std::size_t storage::get_total_samples_count(const server::document_id& document_id)
{
    auto& document = impl_->get_document_storage(document_id);
    return document.get_total_samples_count();
}

const analysis::stacks_analysis& storage::get_stacks_analysis(const server::document_id&  document_id,
                                                              analysis::unique_process_id process_id)
{
    auto& document = impl_->get_document_storage(document_id);
    return document.get_process_analysis(process_id);
}

std::span<const analysis::function_info::id_t> storage::get_functions_page(const server::document_id&  document_id,
                                                                           analysis::unique_process_id process_id,
                                                                           function_data_type sort_by, bool reversed,
                                                                           std::size_t page_size, std::size_t page_index)
{
    auto& document = impl_->get_document_storage(document_id);

    const auto& sorted_functions = document.get_sorted_functions(process_id, sort_by);

    const auto page_start = std::min(page_index * page_size, sorted_functions.size());
    const auto page_end   = std::min((page_index + 1) * page_size, sorted_functions.size());

    const auto current_page_size = page_end - page_start;

    return reversed ? std::span(sorted_functions.data() + sorted_functions.size() - page_end, current_page_size) :
                      std::span(sorted_functions.data() + page_start, current_page_size);
}
