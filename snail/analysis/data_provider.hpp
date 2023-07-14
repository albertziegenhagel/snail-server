#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <snail/common/generator.hpp>

#include <snail/common/types.hpp>

#include <snail/analysis/data/process.hpp>
#include <snail/analysis/data/session.hpp>
#include <snail/analysis/data/stack.hpp>
#include <snail/analysis/data/system.hpp>
#include <snail/analysis/data/thread.hpp>

#include <snail/analysis/options.hpp>
#include <snail/analysis/path_map.hpp>

namespace snail::analysis {

struct sample_data
{
    virtual ~sample_data() = default;

    virtual common::generator<stack_frame> reversed_stack() const = 0;
};

class data_provider
{
public:
    virtual ~data_provider() = default;

    virtual void process(const std::filesystem::path& file_path) = 0;

    virtual const analysis::session_info& session_info() const = 0;

    virtual const analysis::system_info& system_info() const = 0;

    virtual common::generator<common::process_id_t> sampling_processes() const = 0;

    virtual analysis::process_info process_info(common::process_id_t process_id) const = 0;

    virtual common::generator<analysis::thread_info> threads_info(common::process_id_t process_id) const = 0;

    virtual common::generator<const sample_data&> samples(common::process_id_t process_id) const = 0;
};

std::unique_ptr<data_provider> make_data_provider(const std::filesystem::path& extension,
                                                  analysis::options            options         = {},
                                                  path_map                     module_path_map = {});

} // namespace snail::analysis