#pragma once

#include <filesystem>
#include <memory>

#include <snail/common/types.hpp>

namespace snail::analysis {

struct stacks_analysis;
struct process_info;

} // namespace snail::analysis

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

    const std::vector<analysis::process_info>& retrieve_processes(const document_id& id);

    const analysis::stacks_analysis& get_analysis_result(const document_id& id, common::process_id_t process_id);

private:
    struct impl;

    std::unique_ptr<impl> impl_;
};

} // namespace snail::server
