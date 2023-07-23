#include <gtest/gtest.h>

#include <folders.hpp>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    snail::detail::tests::parse_command_line(argc, argv);

    return RUN_ALL_TESTS();
}
