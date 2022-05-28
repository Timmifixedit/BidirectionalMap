#include <iostream>
#include <string>
#include "bidirectional_map.hpp"

template<typename MAP>
void print(const MAP &m, const std::string &name = "") {
    std::cout << name << std::endl;
    for (const auto &[v1, v2] : m) {
        std::cout << "val1: " << v1 << ", val2: " << v2 << std::endl;
    }
}

struct String {
    std::string s;
    struct Hash {
        std::size_t operator()(const String &string) const {
            return std::hash<std::string>()(string.s);
        }
    };

    bool operator==(const String &string) const {
        return s == string.s;
    }
};

template<typename T, typename U>
using StringMap = std::unordered_map<T, U, String::Hash>;

int main() {
    bimap::bidirectional_map<std::string, int> test;
    test.emplace("hallo", 7);
    test.inverse().emplace(8, "moin");
    print(test, "test");
    auto copy = test;
    print(copy, "copy");
    copy.emplace("hallo", 8);
    print(copy, "copy");
    copy.erase(copy.find("hallo"));
    print(copy, "copy");
    print(test, "test");
    copy = std::move(test);
    print(copy, "copy");
    copy.emplace("abc", 26);
    print(copy, "copy");
}


