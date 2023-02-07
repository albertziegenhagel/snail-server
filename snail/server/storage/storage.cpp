
#include <snail/server/storage/storage.hpp>

#include <unordered_map>

#include <snail/analysis/analysis.hpp>
#include <snail/analysis/etl_stack_provider.hpp>
#include <snail/analysis/perf_data_stack_provider.hpp>

using namespace snail;
using namespace snail::server;

namespace {

struct document_storage
{
    std::filesystem::path                     path;
    std::unique_ptr<analysis::stack_provider> stack_provider;

    bool                                                                has_data = false;
    std::vector<analysis::process_info>                                 processes;
    std::unordered_map<common::process_id_t, analysis::stacks_analysis> data;
};

void do_analysis(document_storage& data)
{
    data.processes.clear();
    data.data.clear();
    for(const auto process_id : data.stack_provider->processes())
    {
        data.data[process_id] = snail::analysis::analyze_stacks(*data.stack_provider, process_id);

        data.processes.push_back(data.data[process_id].process);
    }
    data.has_data = true;
}

} // namespace

struct storage::impl
{
    std::unordered_map<std::size_t, document_storage> open_documents;

    document_storage& get_document_storage(const document_id& id, bool requires_data = true)
    {
        auto iter = open_documents.find(id.id_);
        if(iter == open_documents.end()) throw std::runtime_error("invalid document id");

        if(requires_data && !iter->second.has_data)
        {
            do_analysis(iter->second);
        }

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

document_id storage::read_document(const std::filesystem::path& path)
{
    const auto extension = path.extension();

    std::unique_ptr<snail::analysis::stack_provider> stack_provider;
    if(extension == ".etl")
    {
        auto provider = std::make_unique<snail::analysis::etl_stack_provider>(path);
        provider->process();
        stack_provider = std::move(provider);
    }
    else if(extension == ".data")
    {
        auto provider = std::make_unique<snail::analysis::perf_data_stack_provider>(path);
        provider->process();
        stack_provider = std::move(provider);
    }
    else
    {
        throw std::runtime_error("Unsupported file extension");
    }

    const auto new_id                 = impl_->take_document_id();
    impl_->open_documents[new_id.id_] = document_storage{
        .path           = path,
        .stack_provider = std::move(stack_provider)};

    return new_id;
}

void storage::close_document(const document_id& id)
{
    auto iter = impl_->open_documents.find(id.id_);
    if(iter == impl_->open_documents.end()) return;

    impl_->open_documents.erase(iter);
}

const std::vector<analysis::process_info>& storage::retrieve_processes(const document_id& id)
{
    auto& document = impl_->get_document_storage(id);
    return document.processes;
}

const analysis::stacks_analysis& storage::get_analysis_result(const document_id&   id,
                                                              common::process_id_t process_id)
{
    auto& document = impl_->get_document_storage(id);

    auto iter = document.data.find(process_id);
    if(iter == document.data.end()) throw std::runtime_error("invalid process id");

    return iter->second;
}
