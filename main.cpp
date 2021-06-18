#include <iostream>
#include <string>
#include "BidirectionalMap.hpp"

template<typename MAP>
void print(const MAP &m) {
    for (const auto &[v1, v2] : m) {
        std::cout << "val1: " << v1 << ", val2: " << v2 << std::endl;
    }
}

int main() {
    BiMap::BidirectionalMap<std::string, int> test;
    test.emplace("hallo", 7);
    test.invert().emplace(8, "moin");
    print(test);
    auto it = test.begin();
    std::cout << it->second << std::endl;
    it = test.erase(it);
    print(test);
    print(test.invert());
    const int &elem = it->second;
    ++it;
    std::cout << elem << std::endl;
    if (it != test.end()) {
        std::cout << it->first << std::endl;
    }
}

