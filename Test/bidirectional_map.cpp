//
// Created by tim on 21.06.21.
//
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <exception>

#include "bidirectional_map.hpp"
#include "TestUtil.hpp"

template<typename It, typename T, typename U>
void checkValues(It it, const T& first, const U& second) {
    EXPECT_EQ(it->first, first);
    EXPECT_EQ(it->second, second);
}

TEST(BidirectionalMap, ctor) {
    using namespace bimap;
    bidirectional_map<std::string, int> test;
    EXPECT_TRUE(test.empty());
    EXPECT_EQ(test.size(), 0);
}

TEST(BidirectionalMap, emplace) {
    using namespace bimap;
    bidirectional_map<std::string, int> test;
    auto [it, _] = test.emplace("Test", 123);
    EXPECT_EQ(test.size(), 1);
    EXPECT_FALSE(test.empty());
    checkValues(it, "Test", 123);
}

TEST(BidirectionalMap, unique_items) {
    using namespace bimap;
    bidirectional_map<std::string, int> test;
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
    using namespace bimap;
    bidirectional_map<std::string, int> test;
    test.emplace("Test", 123);
    test.emplace("NewItem", 456);
    auto it = test.find("Test");
    checkValues(it, "Test", 123);
    it = test.find("Stuff");
    EXPECT_EQ(it, test.end());
}

TEST(BidirectionalMap, ctor_initializer) {
    using namespace bimap;
    bidirectional_map<std::string, int> test = {{"Test", 1}, {"SecondItem", 2}};
    EXPECT_FALSE(test.empty());
    EXPECT_EQ(test.size(), 2);
    checkValues(test.find("Test"), "Test", 1);
    checkValues(test.find("SecondItem"), "SecondItem", 2);
}

TEST(BidirectionalMap, contains) {
    using namespace bimap;
    bidirectional_map<std::string, int> test = {{"Test", 123}, {"NewItem", 456}};
    EXPECT_TRUE(test.contains("Test"));
    EXPECT_TRUE(test.contains("NewItem"));
    EXPECT_FALSE(test.contains("abc"));
}

TEST(BidirectionalMap, ctor_initialize_from_container) {
    using namespace bimap;
    std::unordered_map<std::string, int> tmp = {{"Test", 1}, {"SecondItem", 2}};
    auto tmpCopy = tmp;
    bidirectional_map<std::string, int> test(tmp.begin(), tmp.end());
    EXPECT_EQ(tmp, tmpCopy);
    EXPECT_FALSE(test.empty());
    EXPECT_EQ(test.size(), 2);
    checkValues(test.find("Test"), "Test", 1);
    checkValues(test.find("SecondItem"), "SecondItem", 2);
}

TEST(BidirectionalMap, erase) {
    using namespace bimap;
    bidirectional_map<std::string, int> test;
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
    using namespace bimap;
    bidirectional_map<std::string, int> test;
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
    using namespace bimap;
    bidirectional_map<std::string, int, std::map> test = {{"Item1", 123}, {"Item2", 456}, {"Item3", 789},
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
    using namespace bimap;
    bidirectional_map<std::string, int, std::map> test = {{"Item1", 123}, {"Item2", 456}, {"Item3", 789}};
    std::unordered_map<std::string, int> lookup = {{"Item1", 123}, {"Item2", 456}, {"Item3", 789}};
    for (auto it = test.begin(); it != test.end(); ++it) {
        auto lookupRes = lookup.find(it->first);
        checkValues(it, lookupRes->first, lookupRes->second);
        lookup.erase(it->first);
    }

    EXPECT_TRUE(lookup.empty());
}

TEST(BidirectionalMap, iterate_empty) {
    using namespace bimap;
    bidirectional_map<std::string, int> test;
    EXPECT_EQ(test.begin(), test.end());
}

TEST(BidirectionalMap, iterate_with_erase) {
    using namespace bimap;
    bidirectional_map<std::string, int, std::map> test = {{"Item1", 123}, {"Item2", 456}, {"Item3", 789}};
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

TEST(BidirectionalMap, bidirectional_iteration) {
    using namespace bimap;
    bidirectional_map<std::string, int, std::map> test = {{"Item1", 123}, {"Item2", 456}, {"Item3", 789}};
    const auto start = test.begin();
    auto curr = start;
    ++curr;
    checkValues(curr++, "Item2", 456);
    EXPECT_EQ(++curr, test.end());
    --curr;
    checkValues(--curr, "Item2", 456);
    checkValues(curr--, "Item2", 456);
}

TEST(BidirectionalMap, comparison) {
    using namespace bimap;
    bidirectional_map<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    bidirectional_map<std::string, int> test1 = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    bidirectional_map<std::string, int> test2 = {{"Test", 123}, {"Stuff", 789}};
    bidirectional_map<std::string, int> test3 = {{"Test", 0}, {"NewItem", 456}, {"Stuff", 789}};
    bidirectional_map<std::string, int> test4 = {{"Testing", 123}, {"NewItem", 456}, {"Stuff", 789}};
    EXPECT_EQ(original, test1);
    EXPECT_NE(original, test2);
    EXPECT_NE(original, test3);
    EXPECT_NE(original, test4);
}

TEST(BidirectionalMap, copy_ctor) {
    using namespace bimap;
    bidirectional_map<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto copy = original;
    EXPECT_EQ(original, copy);
    original.emplace("AddStuff", 17);
    EXPECT_EQ(copy.size(), 3);
    EXPECT_EQ(copy.find("AddStuff"), copy.end());
    copy.emplace("CopyItem", 17);
    EXPECT_EQ(original.size(), 4);
    EXPECT_EQ(original.find("CopyItem"), original.end());
}

TEST(BidirectionalMap, copy_ctor_elements) {
    using namespace bimap;
    bidirectional_map<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto copy = original;
    auto origIt = original.begin();
    auto copyIt = copy.begin();
    while (origIt != original.end()) {
        EXPECT_NE(&origIt->first, &copyIt->first);
        EXPECT_NE(&origIt->second, &copyIt->second);
        ++origIt; ++copyIt;
    }
}

TEST(BidirectionalMap, move_ctor) {
    using namespace bimap;
    bidirectional_map<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto copy = original;
    auto moved = std::move(original);
    EXPECT_EQ(moved, copy);
    moved.emplace("AnotherItem", 17);
    EXPECT_EQ(moved.size(), 4);
    checkValues(moved.find("AnotherItem"), "AnotherItem", 17);
}

TEST(BidirectionalMap, assignment) {
    using namespace bimap;
    bidirectional_map<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    bidirectional_map<std::string, int> overwritten  = {{"abc", 1}};
    overwritten = original;
    EXPECT_EQ(overwritten, original);
    original.emplace("AddStuff", 17);
    EXPECT_EQ(overwritten.size(), 3);
    EXPECT_EQ(overwritten.find("AddStuff"), overwritten.end());
    auto copy = original;
    overwritten = std::move(original);
    EXPECT_EQ(overwritten, copy);
}

TEST(BidirectionalMap, use_after_move) {
    using namespace bimap;
    bidirectional_map<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto moved = std::move(original);
    EXPECT_TRUE(original.empty());
    EXPECT_EQ(original.begin(), original.end());
    EXPECT_TRUE(original.inverse().empty());
    EXPECT_EQ(original.inverse().begin(), original.inverse().end());
    original.emplace("Test", 123);
    original.inverse().emplace(456, "NewItem");
    EXPECT_EQ(original.size(), 2);
    EXPECT_EQ(original.at("Test"), 123);
    EXPECT_EQ(original.at("NewItem"), 456);
}

TEST(BidirectionalMap, return_inverse) {
    using namespace bimap;
    auto generator = []() -> bidirectional_map<int, std::string> {
        bidirectional_map<std::string, int> map = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
        return move(map.inverse());
    };

    auto test = generator();
    EXPECT_EQ(test.size(), 3);
    EXPECT_EQ(test.at(123), "Test");
    test.emplace(1, "one");
    EXPECT_EQ(test.at(1), "one");
    EXPECT_EQ(test.size(), 4);
}

TEST(BidirectionalMap, swap) {
    using namespace bimap;
    bidirectional_map<std::string, int> map1 = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    bidirectional_map<std::string, int> map2 = {{"One", 1}, {"Two", 2}};
    std::swap(map1, map2);
    EXPECT_EQ(map1.size(), 2);
    EXPECT_EQ(map2.size(), 3);
    EXPECT_EQ(map1.at("One"), 1);
    EXPECT_EQ(map1.at("Two"), 2);
    EXPECT_EQ(map1.inverse().at(1), "One");
    EXPECT_EQ(map1.inverse().at(2), "Two");
    EXPECT_EQ(map2.at("Test"), 123);
    EXPECT_EQ(map2.at("NewItem"), 456);
    EXPECT_EQ(map2.at("Stuff"), 789);
    EXPECT_EQ(map2.inverse().at(123), "Test");
    EXPECT_EQ(map2.inverse().at(456), "NewItem");
    EXPECT_EQ(map2.inverse().at(789), "Stuff");
    auto &same = map2.inverse().inverse();
    EXPECT_EQ(map1, map1.inverse().inverse());
    EXPECT_EQ(map2, map2.inverse().inverse());
}

TEST(BidirectionalMap, move_swap_back_and_forth) {
    using namespace bimap;
    bidirectional_map<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto moved = std::move(original);
    moved.inverse().emplace(0, "zero");
    original = std::move(moved);
    EXPECT_EQ(original.size(), 4);
    EXPECT_EQ(original.at("zero"), 0);
}

TEST(BidirectionalMap, clear) {
    using namespace bimap;
    bidirectional_map<std::string, int> test = {{"Test", 123}};
    test.clear();
    EXPECT_TRUE(test.empty());
    EXPECT_EQ(test.size(), 0);
    EXPECT_EQ(test.find("Test"), test.end());
}

TEST(BidirectionalMap, inverse_access_content) {
    using namespace bimap;
    bidirectional_map<std::string, int> test = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    const auto &inverse = test.inverse();
    EXPECT_EQ(inverse.size(), 3);
    checkValues(inverse.find(123), 123, "Test");
    checkValues(inverse.find(456), 456, "NewItem");
    checkValues(inverse.find(789), 789, "Stuff");
}

TEST(BidirectionalMap, inverse_access_emplace) {
    using namespace bimap;
    bidirectional_map<std::string, int> test = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto &inverse = test.inverse();
    inverse.emplace(17, "Inverse");
    EXPECT_EQ(inverse.size(), 4);
    EXPECT_EQ(test.size(), 4);
    checkValues(test.find("Inverse"), "Inverse", 17);
    checkValues(inverse.find(17), 17, "Inverse");
    auto [it, inserted] = inverse.emplace(123, "bla");
    EXPECT_EQ(inverse.size(), 4);
    EXPECT_EQ(test.size(), 4);
    EXPECT_EQ(test.find("bla"), test.end());
    EXPECT_FALSE(inserted);
    checkValues(it, 123, "Test");
}

TEST(BidirectionalMap, inverse_access_identity) {
    using namespace bimap;
    bidirectional_map<std::string, int> test = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto &same = test.inverse().inverse();
    EXPECT_EQ(test, same);
    same.emplace("abc", 17);
    EXPECT_EQ(test.size(), 4);
    EXPECT_EQ(same.size(), 4);
    checkValues(test.find("abc"), "abc", 17);
    checkValues(same.find("abc"), "abc", 17);
}

TEST(BidirectionalMap, inverse_access_clear) {
    using namespace bimap;
    bidirectional_map<std::string, int> test = {{"Test", 123}};
    auto &inverse = test.inverse();
    test.clear();
    EXPECT_TRUE(inverse.empty());
    EXPECT_EQ(inverse.find(123), inverse.end());
    inverse.emplace(123, "Test");
    inverse.clear();
    EXPECT_TRUE(inverse.empty());
    EXPECT_TRUE(test.empty());
    EXPECT_EQ(inverse.find(123), inverse.end());
    EXPECT_EQ(test.find("Test"), test.end());
}

TEST(BidirectionalMap, inverse_access_erase) {
    using namespace bimap;
    bidirectional_map<std::string, int> test = {{"Test", 123}, {"NewItem", 456}, {"AnotherItem", 789}};
    auto &inverse = test.inverse();
    test.erase("NewItem");
    EXPECT_EQ(inverse.size(), 2);
    EXPECT_EQ(inverse.find(456), inverse.end());
    inverse.erase(123);
    EXPECT_EQ(test.size(), 1);
    EXPECT_EQ(test.find("Test"), test.end());
}


TEST(BidirectionalMap, inverse_access_emplace_after_moved) {
    using namespace bimap;
    bidirectional_map<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto moved = std::move(original);
    moved.inverse().emplace(17, "AnotherItem");
    EXPECT_EQ(moved.inverse().size(), 4);
    checkValues(moved.inverse().find(17), 17, "AnotherItem");
    checkValues(moved.find("AnotherItem"), "AnotherItem", 17);
}

TEST(BidirectionalMap, copy_inverse) {
    using namespace bimap;
    bidirectional_map<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto copy = original.inverse();
    EXPECT_EQ(copy, original.inverse());
    original.emplace("AddStuff", 17);
    EXPECT_EQ(copy.size(), 3);
    EXPECT_EQ(copy.find(17), copy.end());
    copy.emplace(18, "NewCopyItem");
    EXPECT_EQ(original.size(), 4);
    EXPECT_EQ(original.find("NewCopyItem"), original.end());
    copy.inverse().erase("Test");
    EXPECT_EQ(copy.find(123), copy.end());
    checkValues(original.find("Test"), "Test", 123);
}

TEST(BidirectionalMap, move_inverse) {
    using namespace bimap;
    bidirectional_map<std::string, int> original = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    auto moved = std::move(original.inverse());
    moved.emplace(18, "NewMoveItem");
    checkValues(moved.find(18), 18, "NewMoveItem");
    moved.inverse().erase("Test");
    EXPECT_EQ(moved.find(123), moved.end());
    checkValues(moved.find(456), 456, "NewItem");
}

TEST(BidirectionalMap, zero_copy) {
    using namespace bimap;
    bidirectional_map<MustNotCopy, int, MNCMAp> test;
    test.emplace("Test1", 1);
    test.emplace("Test2", 2);
    test.emplace("Test3", 3);
    auto moved = std::move(test);
    auto it = moved.begin();
    while (it != moved.end()) {
        ++it;
    }

    std::vector<std::string> strings;
    for(const auto &[mnc, _] : moved) {
        strings.emplace_back(mnc.s);
    }

    for (const auto &[_, mnc] : moved.inverse()) {
        strings.emplace_back(mnc.s);
    }
}

TEST(BidirectionalMap, at) {
    using namespace bimap;
    bidirectional_map<std::string, int> test = {{"Test", 123}, {"NewItem", 456}, {"Stuff", 789}};
    EXPECT_EQ(test.at("Test"), 123);
    EXPECT_EQ(test.at("Stuff"), 789);
    EXPECT_EQ(test.inverse().at(456), "NewItem");
    EXPECT_THROW(test.at("NotIncluded"), std::out_of_range);
    EXPECT_THROW(test.inverse().at(0), std::out_of_range);
}

TEST(BidirectionalMap, noexcept_iterator) {
    using namespace bimap;
    bidirectional_map<std::string, int, std::map> test;
    auto it = test.begin();
    EXPECT_TRUE(noexcept(++it));
    EXPECT_TRUE(noexcept(it++));
    EXPECT_TRUE(noexcept(--it));
    EXPECT_TRUE(noexcept(it--));
    EXPECT_TRUE(noexcept(it == it));
    EXPECT_TRUE(noexcept(it != it));
    EXPECT_TRUE(std::is_nothrow_copy_constructible_v<decltype(it)>);
    EXPECT_TRUE(std::is_nothrow_copy_assignable_v<decltype(it)>);
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<decltype(it)>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<decltype(it)>);
}

TEST(BidirectionalMap, throwing_iterator) {
    using namespace bimap;
    BadIterator<std::string, const int*> baseIt;
    bidirectional_map<std::string, int, BadContainer>::iterator badIt(baseIt);
    EXPECT_THROW(++badIt, std::runtime_error);
    EXPECT_THROW(badIt++, std::runtime_error);
    EXPECT_THROW(--badIt, std::runtime_error);
    EXPECT_THROW(badIt--, std::runtime_error);
    EXPECT_THROW(badIt == badIt, std::runtime_error);
    EXPECT_THROW(badIt != badIt, std::runtime_error);
    EXPECT_THROW(*badIt, std::runtime_error);
    EXPECT_THROW(badIt->first, std::runtime_error);
    EXPECT_THROW(badIt->second, std::runtime_error);
}

TEST(BidirectionalMap, throwing_base_container) {
    using namespace bimap;
    bidirectional_map<std::string, int, BadMap> test;
    EXPECT_THROW(test.size(), std::runtime_error);
    EXPECT_THROW(test.empty(), std::runtime_error);
    EXPECT_THROW(test.reserve(1), std::runtime_error);
    EXPECT_THROW(test.begin(), std::runtime_error);
    EXPECT_THROW(test.end(), std::runtime_error);
    EXPECT_THROW(test.find(""), std::runtime_error);
    EXPECT_THROW(test.contains(""), std::runtime_error);
    EXPECT_THROW(test.clear(), std::runtime_error);
}