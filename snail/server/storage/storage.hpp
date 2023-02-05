#pragma once

#include <filesystem>
#include <memory>

#include <snail/data/types.hpp>

namespace snail::data {

class call_tree;

struct process_info
{
    process_id_t process_id;
    std::string  name;
};

} // namespace snail::data

namespace snail::server {

struct document_id
{
    std::size_t id_;
};

class storage
{
public:
    storage();
    ~storage();

    document_id read_document(const std::filesystem::path& path);
    void        close_document(const document_id& id);

    const std::vector<data::process_info>& retrieve_processes(const document_id& id);

    const data::call_tree& retrieve_call_tree(const document_id& id, data::process_id_t process_id);

private:
    struct impl;

    std::unique_ptr<impl> impl_;
};

} // namespace snail::server
