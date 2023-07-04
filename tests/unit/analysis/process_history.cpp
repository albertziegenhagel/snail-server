
#include <gtest/gtest.h>

#include <string>

#include <snail/analysis/detail/process_history.hpp>

using namespace snail;
using namespace snail::analysis::detail;

struct name_payload
{
    std::string name;

    [[nodiscard]] friend bool operator==(const name_payload& lhs, const name_payload& rhs)
    {
        return lhs.name == rhs.name;
    }
};

TEST(History, Insert)
{
    history<unsigned int, unsigned int, name_payload> hist;

    EXPECT_TRUE(hist.insert(1, 10, {"entry-a"}));
    EXPECT_FALSE(hist.insert(1, 11, {"entry-a"}));
    EXPECT_TRUE(hist.insert(1, 11, {"entry-b"}));

    EXPECT_EQ(hist.all_entries().size(), 1);
    EXPECT_EQ(hist.all_entries().contains(1), 1);
    EXPECT_EQ(hist.all_entries().at(1).size(), 2);
    EXPECT_EQ(hist.all_entries().at(1)[0].id, 1);
    EXPECT_EQ(hist.all_entries().at(1)[0].timestamp, 10);
    EXPECT_EQ(hist.all_entries().at(1)[0].payload.name, "entry-a");
    EXPECT_EQ(hist.all_entries().at(1)[1].id, 1);
    EXPECT_EQ(hist.all_entries().at(1)[1].timestamp, 11);
    EXPECT_EQ(hist.all_entries().at(1)[1].payload.name, "entry-b");

    EXPECT_TRUE(hist.insert(2, 5, {"entry-c"}));

    EXPECT_EQ(hist.all_entries().size(), 2);
    EXPECT_TRUE(hist.all_entries().contains(1));
    EXPECT_EQ(hist.all_entries().at(1).size(), 2);
    EXPECT_EQ(hist.all_entries().at(1)[0].id, 1);
    EXPECT_EQ(hist.all_entries().at(1)[0].timestamp, 10);
    EXPECT_EQ(hist.all_entries().at(1)[0].payload.name, "entry-a");
    EXPECT_EQ(hist.all_entries().at(1)[1].id, 1);
    EXPECT_EQ(hist.all_entries().at(1)[1].timestamp, 11);
    EXPECT_EQ(hist.all_entries().at(1)[1].payload.name, "entry-b");
    EXPECT_TRUE(hist.all_entries().contains(2));
    EXPECT_EQ(hist.all_entries().at(2).size(), 1);
    EXPECT_EQ(hist.all_entries().at(2)[0].id, 2);
    EXPECT_EQ(hist.all_entries().at(2)[0].timestamp, 5);
    EXPECT_EQ(hist.all_entries().at(2)[0].payload.name, "entry-c");
}

TEST(History, Find)
{
    history<unsigned int, unsigned int, name_payload> hist;

    EXPECT_TRUE(hist.insert(1, 10, {"entry-a"}));
    EXPECT_TRUE(hist.insert(1, 15, {"entry-b"}));

    EXPECT_TRUE(hist.insert(2, 5, {"entry-c"}));

    EXPECT_EQ(hist.find_at(1, 5, true), nullptr);
    EXPECT_NE(hist.find_at(1, 5, false), nullptr);
    EXPECT_EQ(hist.find_at(1, 5, false)->payload.name, "entry-a");

    EXPECT_NE(hist.find_at(1, 10, true), nullptr);
    EXPECT_NE(hist.find_at(1, 10, false), nullptr);
    EXPECT_EQ(hist.find_at(1, 10, false)->payload.name, "entry-a");

    EXPECT_NE(hist.find_at(1, 12, true), nullptr);
    EXPECT_NE(hist.find_at(1, 12, false), nullptr);
    EXPECT_EQ(hist.find_at(1, 12, false)->payload.name, "entry-a");

    EXPECT_NE(hist.find_at(1, 15, true), nullptr);
    EXPECT_NE(hist.find_at(1, 15, false), nullptr);
    EXPECT_EQ(hist.find_at(1, 15, false)->payload.name, "entry-b");

    EXPECT_NE(hist.find_at(1, 18, true), nullptr);
    EXPECT_NE(hist.find_at(1, 18, false), nullptr);
    EXPECT_EQ(hist.find_at(1, 18, false)->payload.name, "entry-b");

    const auto& c_hist = hist;

    EXPECT_EQ(c_hist.find_at(2, 3, true), nullptr);
    EXPECT_NE(c_hist.find_at(2, 3, false), nullptr);
    EXPECT_EQ(c_hist.find_at(2, 3, false)->payload.name, "entry-c");

    EXPECT_NE(c_hist.find_at(2, 5, true), nullptr);
    EXPECT_NE(c_hist.find_at(2, 5, false), nullptr);
    EXPECT_EQ(c_hist.find_at(2, 5, false)->payload.name, "entry-c");

    EXPECT_NE(c_hist.find_at(2, 10, true), nullptr);
    EXPECT_NE(c_hist.find_at(2, 10, false), nullptr);
    EXPECT_EQ(c_hist.find_at(2, 10, false)->payload.name, "entry-c");
}
