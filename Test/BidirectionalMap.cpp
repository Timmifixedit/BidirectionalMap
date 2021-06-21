//
// Created by tim on 21.06.21.
//
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <unordered_map>
#include <map>

#include "BidirectionalMap.hpp"

template<typename It, typename T, typename U>
void checkValues(It it, const T& first, const U& second) {
    EXPECT_EQ(it->first, first);
    EXPECT_EQ(it->second, second);
}

TEST(BidirectionalMap, ctor) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test;
    EXPECT_TRUE(test.empty());
    EXPECT_EQ(test.size(), 0);
}

TEST(BidirectionalMap, emplace) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test;
    auto [it, _] = test.emplace("Test", 123);
    EXPECT_EQ(test.size(), 1);
    EXPECT_FALSE(test.empty());
    checkValues(it, "Test", 123);
}

TEST(BidirectionalMap, unique_items) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test;
    auto [it, inserted] = test.emplace("Test", 123);
    EXPECT_TRUE(inserted);
    checkValues(it, "Test", 123);
    std::tie(it, inserted) = test.emplace("NewItem", 456);
    EXPECT_TRUE(inserted);
    checkValues(it, "NewItem", 456);
    std::tie(it, inserted) = test.emplace("Test", 123);
    EXPECT_FALSE(inserted);
    checkValues(it, "Test", 123);
    std::tie(it, inserted) = test.emplace("Test", 765);
    EXPECT_FALSE(inserted);
    checkValues(it, "Test", 123);
    std::tie(it, inserted) = test.emplace("EqualInverseKey", 456);
    EXPECT_FALSE(inserted);
    checkValues(it, "NewItem", 456);
}

TEST(BidirectionalMap, find) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test;
    test.emplace("Test", 123);
    test.emplace("NewItem", 456);
    auto it = test.find("Test");
    checkValues(it, "Test", 123);
    it = test.find("Stuff");
    EXPECT_EQ(it, test.end());
}

TEST(BidirectionalMap, erase) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test;
    test.emplace("Test", 123);
    test.emplace("NewItem", 456);
    test.erase(test.find("NewItem"));
    EXPECT_EQ(test.size(), 1);
    EXPECT_EQ(test.find("NewItem"), test.end());
    checkValues(test.find("Test"), "Test", 123);
    EXPECT_EQ(test.erase(test.end()), test.end());
    EXPECT_EQ(test.size(), 1);
    EXPECT_EQ(test.find("NewItem"), test.end());
}

TEST(BidirectionalMap, erase_by_key) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test;
    test.emplace("Test", 123);
    test.emplace("NewItem", 456);
    EXPECT_EQ(test.erase("NewItem"), 1);
    EXPECT_EQ(test.size(), 1);
    EXPECT_EQ(test.find("NewItem"), test.end());
    checkValues(test.find("Test"), "Test", 123);
    EXPECT_EQ(test.erase("Stuff"), 0);
    EXPECT_EQ(test.size(), 1);
    checkValues(test.find("Test"), "Test", 123);
}

TEST(BidirectionalMap, erase_range) {
    using namespace BiMap;
    BidirectionalMap<std::string, int, std::map> test;
    test.emplace("Item1", 123);
    test.emplace("Item2", 456);
    test.emplace("Item3", 789);
    test.emplace("Item4", 1123);
    test.emplace("Item5", 1456);
    test.emplace("Item6", 1789);
    auto start = test.begin();
    ++start;
    auto end = start;
    ++end; ++end; ++end, ++end;
    auto res = test.erase(start, end);
    EXPECT_EQ(test.size(), 2);
    checkValues(test.begin(), "Item1", 123);
    checkValues(res, "Item6", 1789);
}

TEST(BidirectionalMap, iterate) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test;
    std::unordered_map<std::string, int> lookup;
    test.emplace("Item1", 123);
    test.emplace("Item2", 456);
    test.emplace("Item3", 789);
    lookup.emplace("Item1", 123);
    lookup.emplace("Item2", 456);
    lookup.emplace("Item3", 789);
    for (auto it = test.begin(); it != test.end(); ++it) {
        auto lookupRes = lookup.find(it->first);
        checkValues(it, lookupRes->first, lookupRes->second);
        lookup.erase(it->first);
    }

    EXPECT_TRUE(lookup.empty());
}
TEST(BidirectionalMap, iterate_with_erase) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test;
    std::unordered_map<std::string, int> lookup;
    test.emplace("Item1", 123);
    test.emplace("Item2", 456);
    test.emplace("Item3", 789);
    lookup.emplace("Item1", 123);
    lookup.emplace("Item3", 789);
    for (auto it = test.begin(); it != test.end(); ++it) {
        if (it->first == "Item2") {
            it = test.erase(it);
        }

        auto lookupRes = lookup.find(it->first);
        checkValues(it, lookupRes->first, lookupRes->second);
        lookup.erase(it->first);
    }

    EXPECT_TRUE(lookup.empty());
    EXPECT_EQ(test.size(), 2);
}
