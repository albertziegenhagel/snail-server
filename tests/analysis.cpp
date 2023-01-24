
#include <gtest/gtest.h>

#include <ranges>

#include <snail/analysis/etl_stack_provider.hpp>
#include <snail/analysis/call_tree.hpp>

TEST(AnalysisEtl, CallTree)
{
    snail::analysis::etl_stack_provider provider(R"(C:\Users\aziegenhagel\source\snail-support\tests\data\hidden_sc.user_aux.etl)");
    provider.process();

    for(const auto& process : provider.processes())
    {
        const auto call_tree = snail::analysis::build_call_tree(provider, process);
        call_tree.dump_hot_path();
    }
}