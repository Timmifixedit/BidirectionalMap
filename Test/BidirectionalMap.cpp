//
// Created by tim on 21.06.21.
//
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

#include "BidirectionalMap.hpp"

template <typename T, typename U>
void checkValues(typename BiMap::BidirectionalMap<T, U>::Iterator it, const T& first, const U& second) {
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
    checkValues<std::string, int>(it, "Test", 123);
}

TEST(BidirectionalMap, unique_items) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test;
    auto [it, inserted] = test.emplace("Test", 123);
    EXPECT_TRUE(inserted);
    checkValues(it, std::string("Test"), 123);
    std::tie(it, inserted) = test.emplace("NewItem", 456);
    EXPECT_TRUE(inserted);
    checkValues(it, std::string("NewItem"), 456);
    std::tie(it, inserted) = test.emplace("Test", 123);
    EXPECT_FALSE(inserted);
    checkValues(it, std::string("Test"), 123);
    std::tie(it, inserted) = test.emplace("Test", 765);
    EXPECT_FALSE(inserted);
    checkValues(it, std::string("Test"), 123);
    std::tie(it, inserted) = test.emplace("EqualInverseKey", 456);
    EXPECT_FALSE(inserted);
    checkValues(it, std::string("NewItem"), 456);
}
