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

TEST(BidirectionalMap, ctor_initializer) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test = {{"Test", 1}, {"SecondItem", 2}};
    EXPECT_FALSE(test.empty());
    EXPECT_EQ(test.size(), 2);
    checkValues(test.find("Test"), "Test", 1);
    checkValues(test.find("SecondItem"), "SecondItem", 2);
}

TEST(BidirectionalMap, ctor_initialize_from_container) {
    using namespace BiMap;
    std::unordered_map<std::string, int> tmp = {{"Test", 1}, {"SecondItem", 2}};
    auto tmpCopy = tmp;
    BidirectionalMap<std::string, int> test(tmp.begin(), tmp.end());
    EXPECT_EQ(tmp, tmpCopy);
    EXPECT_FALSE(test.empty());
    EXPECT_EQ(test.size(), 2);
    checkValues(test.find("Test"), "Test", 1);
    checkValues(test.find("SecondItem"), "SecondItem", 2);
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
    BidirectionalMap<std::string, int, std::map> test = {{"Item1", 123}, {"Item2", 456}, {"Item3", 789},
                                                         {"Item4", 1123}, {"Item5", 1456}, {"Item6", 1789}};
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
    BidirectionalMap<std::string, int, std::map> test = {{"Item1", 123}, {"Item2", 456}, {"Item3", 789}};
    std::unordered_map<std::string, int> lookup = {{"Item1", 123}, {"Item2", 456}, {"Item3", 789}};
    for (auto it = test.begin(); it != test.end(); ++it) {
        auto lookupRes = lookup.find(it->first);
        checkValues(it, lookupRes->first, lookupRes->second);
        lookup.erase(it->first);
    }

    EXPECT_TRUE(lookup.empty());
}

TEST(BidirectionalMap, iterate_with_erase) {
    using namespace BiMap;
    BidirectionalMap<std::string, int, std::map> test = {{"Item1", 123}, {"Item2", 456}, {"Item3", 789}};
    std::unordered_map<std::string, int> lookup = {{"Item1", 123}, {"Item3", 789}};
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

TEST(BidirectionalMap, comparison) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    BidirectionalMap<std::string, int> test1 = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    BidirectionalMap<std::string, int> test2 = {{"Test", 123}, {"Stuff", 789}};
    BidirectionalMap<std::string, int> test3 = {{"Test", 0}, {"NewItem", 456}, {"Stuff", 789}};
    BidirectionalMap<std::string, int> test4 = {{"Testing", 123}, {"NewItem", 456}, {"Stuff", 789}};
    EXPECT_EQ(original, test1);
    EXPECT_NE(original, test2);
    EXPECT_NE(original, test3);
    EXPECT_NE(original, test4);
}

TEST(BidirectionalMap, copy_ctor) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto copy = original;
    EXPECT_EQ(original, copy);
    original.emplace("AddStuff", 17);
    EXPECT_EQ(copy.size(), 3);
    EXPECT_EQ(copy.find("AddStuff"), copy.end());
}

TEST(BidirectionalMap, move_ctor) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto copy = original;
    auto moved = std::move(original);
    EXPECT_EQ(moved, copy);
}

TEST(BidirectionalMap, asignment) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    BidirectionalMap<std::string, int> overwritten  = {{"abc", 1}};
    overwritten = original;
    EXPECT_EQ(overwritten, original);
    original.emplace("AddStuff", 17);
    EXPECT_EQ(overwritten.size(), 3);
    EXPECT_EQ(overwritten.find("AddStuff"), overwritten.end());
    auto copy = original;
    overwritten = std::move(original);
    EXPECT_EQ(overwritten, copy);
}

TEST(BidirectionalMap, clear) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test = {{"Test", 123}};
    test.clear();
    EXPECT_TRUE(test.empty());
    EXPECT_EQ(test.size(), 0);
    EXPECT_EQ(test.find("Test"), test.end());
}

TEST(BidirectionalMap, inverse_access_content) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    const auto &inverse = test.invert();
    EXPECT_EQ(inverse.size(), 3);
    checkValues(inverse.find(123), 123, "Test");
    checkValues(inverse.find(456), 456, "NewItem");
    checkValues(inverse.find(789), 789, "Stuff");
}

TEST(BidirectionalMap, inverse_access_emplace) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto &inverse = test.invert();
    inverse.emplace(17, "Inverse");
    EXPECT_EQ(inverse.size(), 4);
    EXPECT_EQ(test.size(), 4);
    checkValues(test.find("Inverse"), "Inverse", 17);
    checkValues(inverse.find(17), 17, "Inverse");
}

TEST(BidirectionalMap, inverse_access_identity) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto &same = test.invert().invert();
    EXPECT_EQ(test, same);
    same.emplace("abc", 17);
    EXPECT_EQ(test.size(), 4);
    checkValues(test.find("abc"), "abc", 17);
}
