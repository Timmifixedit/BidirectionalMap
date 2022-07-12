//
// Created by tim on 22.06.21.
//

#include "TestUtil.hpp"

MustNotCopy::MustNotCopy(std::string string) : s(std::move(string)) {}

MustNotCopy::MustNotCopy(const char *chars) : MustNotCopy(std::string(chars)) {}

MustNotCopy::MustNotCopy(const MustNotCopy &other) : s(other.s) {
    // Hack because FAIL() macro is a non-void return statement
    EXPECT_TRUE(false) << "Copy occurred";
}

bool MustNotCopy::operator==(const MustNotCopy &other) const {
    return s == other.s;
}

bool MustNotCopy::operator!=(const MustNotCopy &other) const {
    return !(*this == other);
}

