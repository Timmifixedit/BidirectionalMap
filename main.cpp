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
    BiMap::BidirectionalMap<std::string, int> bidirectionalMap;
    bidirectionalMap->emplace("Hallo", 1);
    bidirectionalMap->invert().emplace(2, "Hello");
    std::cout << bidirectionalMap->size() << std::endl;
    auto bmCopy = bidirectionalMap;
    bmCopy->erase(bmCopy->begin());
    bmCopy->emplace("copy insert", 5);
    print(*bidirectionalMap);
    std::cout << "copy:" << std::endl;
    print(*bmCopy);
    auto moved = std::move(bmCopy);
    std::cout << bmCopy->size() << std::endl;
    BiMap::BidirectionalMap converted = moved->invert();
    converted->clear();
    print(*moved);
    print(moved);
}

