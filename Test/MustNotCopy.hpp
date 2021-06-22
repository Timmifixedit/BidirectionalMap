//
// Created by tim on 22.06.21.
//

#ifndef BIDIRECTIONALMAP_MUSTNOTCOPY_HPP
#define BIDIRECTIONALMAP_MUSTNOTCOPY_HPP

#include <string>
#include <gtest/gtest.h>
#include <unordered_map>

class MustNotCopy {
public:
    MustNotCopy(std::string string);
    MustNotCopy(const char *chars);
    MustNotCopy() = default;
    MustNotCopy(const MustNotCopy &other);

    MustNotCopy(MustNotCopy &&) = default;
    MustNotCopy &operator=(const MustNotCopy &) = default;
    MustNotCopy &operator=(MustNotCopy &&) = default;

    operator std::string () {
        return s;
    }

    bool operator==(const MustNotCopy &other) const;

    bool operator!=(const MustNotCopy &other) const;

    struct Hash {
        std::size_t operator()(const MustNotCopy &m) const {
            return std::hash<std::string>()(m.s);
        }
    };

    std::string s;
};

template<typename T, typename U>
using MNCMAp = std::unordered_map<T, U, MustNotCopy::Hash>;

#endif //BIDIRECTIONALMAP_MUSTNOTCOPY_HPP
