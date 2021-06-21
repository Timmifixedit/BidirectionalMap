//
// Created by tim on 21.06.21.
//
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

#include "BidirectionalMap.hpp"


TEST(BidirectionalMap, Ctor) {
    using namespace BiMap;
    BidirectionalMap<std::string, int> test;
    EXPECT_TRUE(test.empty());
    EXPECT_EQ(test.size(), 0);
}

