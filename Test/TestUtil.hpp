//
// Created by tim on 22.06.21.
//

#ifndef BIDIRECTIONALMAP_TESTUTIL_HPP
#define BIDIRECTIONALMAP_TESTUTIL_HPP

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
struct BadIterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::pair<T, U>;
    using reference = std::add_lvalue_reference_t<std::add_const_t<value_type>>;
    using pointer = std::add_pointer_t<value_type>;
    using difference_type = std::size_t;

    BadIterator &operator++() {
        throw std::runtime_error("This exception should be thrown");
    }

    BadIterator operator++(int) {
        throw std::runtime_error("This exception should be thrown");
    }

    BadIterator &operator--() {
        throw std::runtime_error("This exception should be thrown");
    }

    BadIterator operator--(int) {
        throw std::runtime_error("This exception should be thrown");
    }

    bool operator==(const BadIterator &) const {
        throw std::runtime_error("This exception should be thrown");
    }

    bool operator!=(const BadIterator &) const {
        throw std::runtime_error("This exception should be thrown");
    }

    reference operator*() const noexcept {
        throw std::runtime_error("This exception should be thrown");
    }

    pointer operator->() const {
        throw std::runtime_error("This exception should be thrown");
    }
};

template<typename T, typename U>
struct BadContainer {
    auto begin() const {
        return BadIterator<T, U>{};
    }
};

template<typename T, typename U>
using MNCMAp = std::unordered_map<T, U, MustNotCopy::Hash>;

template<typename T, typename U>
struct BadMap {
    std::size_t size() const {
        throw std::runtime_error("This exception should be thrown");
    }

    bool empty() const {
        throw std::runtime_error("This exception should be thrown");
    }

    void reserve(std::size_t) {
        throw std::runtime_error("This exception should be thrown");
    }

    int* begin() const {
        throw std::runtime_error("This exception should be thrown");
    }

    int* end() const {
        throw std::runtime_error("This exception should be thrown");
    }

    int* find(const T&) const {
        throw std::runtime_error("This exception should be thrown");
    }

    void clear() {
        throw std::runtime_error("This exception should be thrown");
    }
};

#endif //BIDIRECTIONALMAP_TESTUTIL_HPP
