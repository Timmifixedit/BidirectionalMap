# bidirectional_map
Implementation of a bidirectional associative container in c++. Its goal is to behave
similarly to popular stl containers like `std::unordered_map` while providing efficient
lookup from key to value as well as from value to key.
## Properties
The `bidirectional_map` container contains pairs of values of type K1 and K2.
* Objects in the container are immutable, neither values of type K1 nor values of type
  K2 can be modified to ensure the integrity of the underlying associative containers
* The container supports the use of different associative containers as base. The
  default base container is `std::unordered_map` for both forward and inverse lookup.
  Other tested containers are `std::map` as well as `std::multimap` and
  `std::unordered_multimap`.
* The mapping from values of K1 to values of K2 is enforced to be injective if the
  underlying containers for both forward and inverse lookup contain unique keys (like
  in the default case). This means that for example two pairs (k1, k2) and (k1', k2')
  can only be inserted at the same time if k1 != k1' **and** k2 != k2'. The use of
  multimaps as base containers relaxes this constraint.
  
## Code Example
An instance of `bidirectional_map` can be created similarly to `std::unordered_map`:
```c++
#include <string>
#include <unordered_map>
#include "bidirectional_map.hpp"

// empty container
bimap::bidirectional_map<std::string, int> map;

// using initializer list
bimap::bidirectional_map<std::string, int> map1 = {{"Test", 1}, {"Hello", 2}}; 

// from same container type
bimap::bidirectional_map<std::string, int> map3(map1.begin(), map1.end());

// from different container type
std::unordered_map<std::string, int> values = {{"abc", 1}, {"def", 2}};
bimap::bidirectional_map<std::string, int> map2(values.begin(), values.end());
```
From the items used for initialization only unique ones are inserted (see properties).
Further items can be inserted using the `emplace` method
```c++
bimap::bidirectional_map<std::string, int> map;
map.emplace("NewItem", 17); // constructs new item in place

// no insertion. 17 already exists in the container. Returns iterator to ("NewItem", 17)
auto [iterator, inserted] = map.emplace("AnotherItem", 17);
```
Item lookup:
```c++
bimap::bidirectional_map<std::string, int> map{{"NewItem", 12}, {"Stuff", 17}};
auto location = map.find("NewItem");
if (location != map.end()) {
    std::cout << location->first << std::endl;
}

if (map.contains("Stuff")) {
    std::cout << map.at("Stuff") << std::endl; // at is only available when using underlying container that enforces
                                               // unique keys
}
```

### Inverse Access
Using the `inverse()` member, inverse lookup and insertion is possible:
```c++
bimap::bidirectional_map<std::string, int> map;
map.inverse().emplace(123, "one two three"); // inverse insertion
auto invLocation = map.inverse().find(123); // inverse lookup
std::cout << invLocation->first << std::endl; // prints '123'

// inverse of inverse() is again the original
auto location = map.inverse().inverse().find("one two three");
```
`inverse()` returns a reference to `bidirectional_map` where the template types K1 and K2
are reversed. It behaves exactly like the original map except... well the other way around.
Even the iterator members are reversed. Copying the `inverse()` container is allowed and
will copy the container contents. Moving from `inverse()` is also allowed and behaves as
expected.
```c++
bimap::bidirectional_map<std::string, int> map; // map from std::string -> int
auto inverse = map.inverse(); // independent (copied) container of reversed type (int -> string)
auto &inverseRef = map.inverse(); // inverse access to the same container
```

### Custom Map Base Container
It is possible to specify a custom map base container for forward lookup as well as for
inverse lookup. The default map base type is `std::unordered_map` for forward access as
well as for inverse access. Another possible map base type is `std::map`:
```c++
// only forward access uses the ordered map std::map.
// Inverse access is till provided through std::unordered_map
bimap::bidirectional_map<std::string, int, std::map> map;

// Both forward and inverse access use std::map
bimap::bidirectional_map<std::string, int, std::map, std::map> map1;
```
Another scenario for using a different map base type is when you need to specify for
example a custom hash function:
```c++
struct MyString {...}; // Custom data structure with no default std::hash specialization
struct MyHash {...}; // Custom hash struct
struct MyComparator {...}; // Custom comparator necessary for std::unordered_map

template<typename T, typename U>
using BaseMap = std::unordered_map<T, U, MyHash, MyComparator>;

// for inverse access the default std::unordered_map is sufficient
bimap::bidirectional_map<MyString, int, BaseMap> map;
```
