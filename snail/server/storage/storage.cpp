
#include <snail/server/storage/storage.hpp>

#include <unordered_map>

#include <snail/analysis/call_tree.hpp>
#include <snail/analysis/etl_stack_provider.hpp>
#include <snail/analysis/perf_data_stack_provider.hpp>

using namespace snail;
using namespace snail::server;

namespace {

struct document_storage
{
    std::filesystem::path                     path;
    std::unique_ptr<analysis::stack_provider> stack_provider;

    bool                                                    has_data = false;
    std::vector<data::process_info>                         processes;
    std::unordered_map<data::process_id_t, data::call_tree> call_trees;
};

void do_analysis(document_storage& data)
{
    data.processes.clear();
    data.call_trees.clear();
    for(const auto& process : data.stack_provider->processes())
    {
        data.processes.push_back(data::process_info{
            .process_id = process.process_id(),
            .name       = std::string(process.image_name())});

        data.call_trees[process.process_id()] = snail::analysis::build_call_tree(*data.stack_provider, process);
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

const std::vector<data::process_info>& storage::retrieve_processes(const document_id& id)
{
    auto& document = impl_->get_document_storage(id);
    return document.processes;
}

const data::call_tree& storage::retrieve_call_tree(const document_id& id,
                                                   data::process_id_t process_id)
{
    auto& document = impl_->get_document_storage(id);

    auto iter = document.call_trees.find(process_id);
    if(iter == document.call_trees.end()) throw std::runtime_error("invalid process id");

    return iter->second;
}