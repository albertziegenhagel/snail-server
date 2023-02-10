
#include <ranges>
#include <string>

#include <snail/analysis/analysis.hpp>
#include <snail/analysis/data_provider.hpp>

int main(int argc, char* argv[])
{
    if(argc != 2) return EXIT_FAILURE;

    const auto file_path = std::filesystem::path(argv[1]);

    auto provider = snail::analysis::make_data_provider(file_path.extension());
    provider->process(file_path);

    for(const auto process_id : provider->sampling_processes())
    {
        const auto result = snail::analysis::analyze_stacks(*provider, process_id);
        // call_tree.dump_hot_path();
    }

    return EXIT_SUCCESS;
}