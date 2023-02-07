
#include <ranges>
#include <string>

#include <snail/analysis/analysis.hpp>
#include <snail/analysis/etl_stack_provider.hpp>
#include <snail/analysis/perf_data_stack_provider.hpp>

int main(int argc, char* argv[])
{
    if(argc != 2) return EXIT_FAILURE;

    if(std::string_view(argv[1]) == "etl")
    {
        snail::analysis::etl_stack_provider provider(R"(C:\Users\aziegenhagel\source\snail-support\tests\data\hidden_sc.user_aux.etl)");
        provider.process();

        for(const auto process_id : provider.processes())
        {
            const auto result = snail::analysis::analyze_stacks(provider, process_id);
            // call_tree.dump_hot_path();
        }
    }
    else if(std::string_view(argv[1]) == "perf")
    {
        snail::analysis::perf_data_stack_provider provider(R"(C:\Users\aziegenhagel\source\snail-support\tests\data\hidden_debug_perf.data)");
        provider.process();

        for(const auto process_id : provider.processes())
        {
            const auto result = snail::analysis::analyze_stacks(provider, process_id);
            // call_tree.dump_hot_path();
        }
    }

    return EXIT_SUCCESS;
}