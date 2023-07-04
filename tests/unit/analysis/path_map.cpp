
#include <gtest/gtest.h>

#include <snail/analysis/path_map.hpp>

using namespace snail;
using namespace snail::analysis;

TEST(PathMap, Map)
{
    path_map map;

    map.add_rule(std::make_unique<simple_path_mapper>(R"(C:\path\a)", R"(C:\path\b)"));
    map.add_rule(std::make_unique<simple_path_mapper>("/home/path/a", "/home/path/b"));
    map.add_rule(std::make_unique<simple_path_mapper>("/home/path/a/more", "/home/path/d"));
    map.add_rule(std::make_unique<regex_path_mapper>(std::regex(R"(my-file\.exe$)"), "other-file.dll"));

    std::string test;

    test = R"(C:\other-path\a)";
    EXPECT_FALSE(map.try_apply(test));
    EXPECT_EQ(test, R"(C:\other-path\a)");

    test = R"(C:\path\b\my-file.exe)";
    EXPECT_TRUE(map.try_apply(test));
    EXPECT_EQ(test, R"(C:\path\b\other-file.dll)");

    test = R"(C:\path\b\my-file.exe.bak)";
    EXPECT_FALSE(map.try_apply(test));
    EXPECT_EQ(test, R"(C:\path\b\my-file.exe.bak)");

    test = R"(C:\path\a\my-file)";
    EXPECT_TRUE(map.try_apply(test));
    EXPECT_EQ(test, R"(C:\path\b\my-file)");

    test = "/home/path/a/my-file";
    EXPECT_TRUE(map.try_apply(test));
    EXPECT_EQ(test, "/home/path/b/my-file");

    test = "/home/path/a/more";
    EXPECT_TRUE(map.try_apply(test));
    EXPECT_EQ(test, "/home/path/b/more");

    test = "/other/home/path/a";
    EXPECT_FALSE(map.try_apply(test));
    EXPECT_EQ(test, "/other/home/path/a");

    const auto& const_map = map;

    test = "/home/path/a/my-file";
    EXPECT_TRUE(const_map.try_apply(test));
    EXPECT_EQ(test, "/home/path/b/my-file");

    path_map map_copy(map);

    test = "/home/path/a/my-file";
    EXPECT_TRUE(map.try_apply(test));
    EXPECT_EQ(test, "/home/path/b/my-file");

    path_map map_moved(std::move(map));

    test = "/home/path/a/my-file";
    EXPECT_TRUE(map_moved.try_apply(test));
    EXPECT_EQ(test, "/home/path/b/my-file");

    path_map map2;
    map2.add_rule(std::make_unique<simple_path_mapper>("/home/path/a", "/home/path/c"));

    test = "/home/path/a/my-file";
    EXPECT_TRUE(map2.try_apply(test));
    EXPECT_EQ(test, "/home/path/c/my-file");

    map2 = map_moved;

    test = "/home/path/a/my-file";
    EXPECT_TRUE(map2.try_apply(test));
    EXPECT_EQ(test, "/home/path/b/my-file");

    path_map map3;
    map3.add_rule(std::make_unique<simple_path_mapper>("/home/path/a", "/home/path/c"));

    test = "/home/path/a/my-file";
    EXPECT_TRUE(map3.try_apply(test));
    EXPECT_EQ(test, "/home/path/c/my-file");

    map3 = std::move(map2);

    test = "/home/path/a/my-file";
    EXPECT_TRUE(map3.try_apply(test));
    EXPECT_EQ(test, "/home/path/b/my-file");
}
