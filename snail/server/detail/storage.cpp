
#include <snail/server/detail/storage.hpp>

#include <algorithm>
#include <ranges>
#include <unordered_map>

#include <snail/analysis/analysis.hpp>
#include <snail/analysis/data_provider.hpp>
#include <snail/analysis/options.hpp>
#include <snail/analysis/path_map.hpp>

using namespace snail;
using namespace snail::server;
using namespace snail::server::detail;

namespace {

struct document_storage
{
    struct analysis_data
    {
        std::optional<analysis::stacks_analysis> stacks_analysis;

        std::optional<std::vector<analysis::function_info::id_t>> functions_by_name;

        struct sample_sort_data
        {
            std::optional<std::vector<analysis::function_info::id_t>> by_self_samples;
            std::optional<std::vector<analysis::function_info::id_t>> by_total_samples;
        };
        std::unordered_map<analysis::sample_source_info::id_t, sample_sort_data> functions_by_samples;
    };

    std::filesystem::path                    path;
    std::unique_ptr<analysis::data_provider> data_provider;
    analysis::sample_filter                  filter;

    std::optional<std::unordered_map<analysis::sample_source_info::id_t, std::size_t>> total_samples_counts;

    std::unordered_map<
        analysis::unique_process_id,
        analysis_data>
        analysis_per_process;

    const std::unordered_map<analysis::sample_source_info::id_t, std::size_t>& get_total_samples_counts()
    {
        if(total_samples_counts.has_value()) return total_samples_counts.value();

        total_samples_counts.emplace();
        auto& new_counts = total_samples_counts.value();

        for(const auto& sample_source : data_provider->sample_sources())
        {
            std::size_t count = 0;

            for(const auto process_id : data_provider->sampling_processes())
            {
                count += data_provider->count_samples(sample_source.id, process_id, filter);
            }

            new_counts[sample_source.id] = count;
        }

        return new_counts;
    }

    const analysis::stacks_analysis& get_process_analysis(analysis::unique_process_id       process_id,
                                                          const common::progress_listener*  progress_listener,
                                                          const common::cancellation_token* cancellation_token)
    {
        auto& data = analysis_per_process[process_id];

        if(data.stacks_analysis == std::nullopt)
        {
            data = analysis_data{
                .stacks_analysis      = snail::analysis::analyze_stacks(*data_provider, process_id, filter,
                                                                        progress_listener, cancellation_token),
                .functions_by_name    = {},
                .functions_by_samples = {},
            };
        }
        return *data.stacks_analysis;
    }

    const std::vector<analysis::function_info::id_t>& get_sorted_functions(analysis::unique_process_id       process_id,
                                                                           sort_by_kind                      sort_by,
                                                                           const common::progress_listener*  progress_listener,
                                                                           const common::cancellation_token* cancellation_token)
    {
        const auto& stacks_analysis = get_process_analysis(process_id, progress_listener, cancellation_token);

        const auto function_ids_view = stacks_analysis.all_functions() |
                                       std::views::transform([](const snail::analysis::function_info& function)
                                                             { return function.id; });

        auto& data = analysis_per_process[process_id];

        return std::visit(
            [&data, &stacks_analysis, &function_ids_view]<typename T>(const T& sort_by) -> const std::vector<analysis::function_info::id_t>&
            {
                if constexpr(std::is_same_v<T, detail::sort_by_name>)
                {
                    if(data.functions_by_name == std::nullopt)
                    {
                        data.functions_by_name = std::vector<snail::analysis::function_info::id_t>(function_ids_view.begin(), function_ids_view.end());
                        std::ranges::sort(*data.functions_by_name, [&stacks_analysis](const snail::analysis::function_info::id_t& lhs, const snail::analysis::function_info::id_t& rhs)
                                          { return stacks_analysis.get_function(lhs).name < stacks_analysis.get_function(rhs).name; });
                    }
                    return *data.functions_by_name;
                }
                if constexpr(std::is_same_v<T, detail::sort_by_samples>)
                {
                    const auto source_id   = sort_by.sample_source_id;
                    auto&      source_data = data.functions_by_samples[source_id];

                    auto& sorted_functions = sort_by.sum == detail::sort_by_samples::sum_type::self ?
                                                 source_data.by_self_samples :
                                                 source_data.by_total_samples;
                    if(sorted_functions == std::nullopt)
                    {
                        sorted_functions = std::vector<snail::analysis::function_info::id_t>(function_ids_view.begin(), function_ids_view.end());
                        switch(sort_by.sum)
                        {
                        default:
                        case detail::sort_by_samples::sum_type::total:
                            std::ranges::sort(*sorted_functions,
                                              [&stacks_analysis, source_id](const snail::analysis::function_info::id_t& lhs, const snail::analysis::function_info::id_t& rhs)
                                              {
                                                  const auto& lhs_function = stacks_analysis.get_function(lhs);
                                                  const auto& rhs_function = stacks_analysis.get_function(rhs);
                                                  const auto  lhs_value    = lhs_function.hits.get(source_id).total;
                                                  const auto  rhs_value    = rhs_function.hits.get(source_id).total;
                                                  return lhs_value < rhs_value || (lhs_value == rhs_value && lhs_function.name < rhs_function.name);
                                              });
                            break;
                        case detail::sort_by_samples::sum_type::self:
                            std::ranges::sort(*sorted_functions,
                                              [&stacks_analysis, source_id](const snail::analysis::function_info::id_t& lhs, const snail::analysis::function_info::id_t& rhs)
                                              {
                                                  const auto& lhs_function = stacks_analysis.get_function(lhs);
                                                  const auto& rhs_function = stacks_analysis.get_function(rhs);
                                                  const auto  lhs_value    = lhs_function.hits.get(source_id).self;
                                                  const auto  rhs_value    = rhs_function.hits.get(source_id).self;
                                                  return lhs_value < rhs_value || (lhs_value == rhs_value && lhs_function.name < rhs_function.name);
                                              });
                            break;
                        }
                    }
                    return *sorted_functions;
                }
            },
            sort_by);
    }
};

} // namespace

struct storage::impl
{
    analysis::options                                 options;
    analysis::path_map                                module_path_map;
    std::unordered_map<document_id, document_storage> open_documents;

    document_storage& get_document_storage(const document_id& id)
    {
        auto iter = open_documents.find(id);
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

document_id storage::create_document(const std::filesystem::path& path)
{
    const auto new_id = impl_->take_document_id();

    impl_->open_documents[new_id] = document_storage{
        .path                 = path,
        .data_provider        = nullptr,
        .filter               = {},
        .total_samples_counts = {},
        .analysis_per_process = {},
    };
    return new_id;
}

void storage::read_document(const document_id&                id,
                            const common::progress_listener*  progress_listener,
                            const common::cancellation_token* cancellation_token)
{
    auto iter = impl_->open_documents.find(id);
    if(iter == impl_->open_documents.end()) return;

    auto& document = impl_->get_document_storage(id);

    auto data_provider = analysis::make_data_provider(document.path.extension(),
                                                      impl_->options,
                                                      impl_->module_path_map);

    data_provider->process(document.path, progress_listener, cancellation_token);

    document.data_provider = std::move(data_provider);
}

void storage::close_document(const document_id& id)
{
    auto iter = impl_->open_documents.find(id);
    if(iter == impl_->open_documents.end()) return;

    impl_->open_documents.erase(iter);
}

const analysis::data_provider& storage::get_data(const detail::document_id& document_id)
{
    auto& document = impl_->get_document_storage(document_id);
    if(document.data_provider == nullptr) throw std::runtime_error("Document has not been read yet.");
    return *document.data_provider;
}

void storage::apply_document_filter(const document_id& document_id, analysis::sample_filter filter)
{
    auto& document = impl_->get_document_storage(document_id);
    if(document.filter == filter) return;
    document.filter = std::move(filter);
    document.total_samples_counts.reset();
    document.analysis_per_process.clear();
}

const analysis::sample_filter& storage::get_document_filter(const document_id& document_id)
{
    auto& document = impl_->get_document_storage(document_id);
    return document.filter;
}

const std::unordered_map<analysis::sample_source_info::id_t, std::size_t>& storage::get_total_samples_counts(const detail::document_id& document_id)
{
    auto& document = impl_->get_document_storage(document_id);
    return document.get_total_samples_counts();
}

const analysis::stacks_analysis& storage::get_stacks_analysis(const detail::document_id&        document_id,
                                                              analysis::unique_process_id       process_id,
                                                              const common::progress_listener*  progress_listener,
                                                              const common::cancellation_token* cancellation_token)
{
    auto& document = impl_->get_document_storage(document_id);
    return document.get_process_analysis(process_id, progress_listener, cancellation_token);
}

std::span<const analysis::function_info::id_t> storage::get_functions_page(const detail::document_id&  document_id,
                                                                           analysis::unique_process_id process_id,
                                                                           sort_by_kind                sort_by,
                                                                           bool                        reversed,
                                                                           std::size_t page_size, std::size_t page_index,
                                                                           const common::progress_listener*  progress_listener,
                                                                           const common::cancellation_token* cancellation_token)
{
    auto& document = impl_->get_document_storage(document_id);

    const auto& sorted_functions = document.get_sorted_functions(process_id, sort_by, progress_listener, cancellation_token);

    const auto page_start = std::min(page_index * page_size, sorted_functions.size());
    const auto page_end   = std::min((page_index + 1) * page_size, sorted_functions.size());

    const auto current_page_size = page_end - page_start;

    return reversed ? std::span(sorted_functions.data() + sorted_functions.size() - page_end, current_page_size) :
                      std::span(sorted_functions.data() + page_start, current_page_size);
}
