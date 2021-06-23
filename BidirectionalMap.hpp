/**
 * @author Tim Luchterhand
 * @date 2021-06-16
 * @brief This file contains the class definition of a bidirectional associative container that can be used for
 * efficient lookup in both directions. Its contents are immutable to ensure the integrity of the underlying map
 * map containers. Also the mapping has to be injective to make bidirectional lookup possible.
 */

#ifndef BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
#define BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP

#include <unordered_map>
#include <optional>

namespace BiMap::implementation {
    /**
     * Very simple pointer class that can be used to allocate storage once but can also be used as a non owning pointer.
     * Unlike shared_ptr, copies of this class are non-owning pointers and unlike weak_ptr, non-owning pointers
     * do not know if the object behind the pointer still exists. The owning pointer deallocates storage at destruction
     * @tparam T type of object behind the pointer
     */
    template<typename T>
    class AllocOncePointer {
    public:
        /**
         * Creates an empty nullptr object
         */
        constexpr AllocOncePointer() noexcept: data(nullptr), owner(false) {}

        /**
         * Creates a non owning pointer to an existing object
         * @param data
         */
        constexpr AllocOncePointer(T *data) noexcept : data(data), owner(false) {}

        /**
         * Allocates storage and creates an instance of T in place. Becomes owner of the storage
         * @tparam ARGS
         * @param args Arguments that are passed to the constructor of T by std::forward
         */
        template<typename ...ARGS>
        explicit AllocOncePointer(ARGS &&...args) : data(new T(std::forward<ARGS>(args)...)), owner(true) {}

        /**
         * Copy constructor, creates a non-owning pointer
         * @param other
         */
        AllocOncePointer(const AllocOncePointer &other) noexcept: data(other.data), owner(false) {}

        /**
         * Swaps pointer and ownership with other
         * @param other
         */
        void swap(AllocOncePointer &other) noexcept {
            std::swap(data, other.data);
            std::swap(owner, other.owner);
        }

        AllocOncePointer(AllocOncePointer &&other) noexcept: AllocOncePointer() {
            swap(other);
        }

        AllocOncePointer &operator=(AllocOncePointer other) noexcept {
            swap(other);
            return *this;
        }

        /**
         * Destructor. Deallocates memory only when owner
         */
        ~AllocOncePointer() {
            if (owner) {
                delete data;
            }
        }

        /**
         * Check if pointer is owner
         * @return
         */
        [[nodiscard]] bool isOwner() const noexcept {
            return owner;
        }

        T &operator*() noexcept {
            return *data;
        }

        const T &operator*() const noexcept {
            return *data;
        }

        T *operator->() noexcept {
            return data;
        }

        const T *operator->() const noexcept {
            return data;
        }

        /**
         * Comparison operator
         * @param other
         * @return true if data pointers point to the same object, false otherwise
         */
        bool operator==(const AllocOncePointer &other) const {
            return data == other.data;
        }

        bool operator==(nullptr_t) const {
            return data == nullptr;
        }

        template<typename Ptr>
        bool operator!=(const Ptr& other) const {
            return !(*this == other);
        }

    private:
        T *data;
        bool owner{};
    };

    /**
     * See member function swap
     * @tparam T
     * @param a
     * @param b
     */
    template<typename T>
    void swap(AllocOncePointer<T> &a, AllocOncePointer<T> &b) noexcept {
        a.swap(b);
    }
}

namespace BiMap {
    /**
     * Bidirectional associative container that supports efficient lookup in both directions. To ensure that lookup in
     * both directions is possible, only unique items of type ForwardKey and InverseKey can be inserted. Neither items
     * of type ForwardKey nor InverseKey can be modified.
     * @tparam ForwardKey Type of key used for forward lookup
     * @tparam InverseKey Type of key used for inverse lookup
     * @tparam ForwardMapType base map container used for forward lookup. Default is std::unordered_map
     * @tparam InverseMapType base map container used for inverse lookup. Default is std::unordered_map
     */
    template<typename ForwardKey, typename InverseKey,
            template<typename T, typename U> typename ForwardMapType = std::unordered_map,
            template<typename T, typename U> typename InverseMapType = std::unordered_map>
    class BidirectionalMap {
    private:
        using ForwardMap = ForwardMapType<ForwardKey, const InverseKey *>;
        using InverseBiMap = BidirectionalMap<InverseKey, ForwardKey, InverseMapType, ForwardMapType>;
        friend InverseBiMap;
        using InversBiMapPtr = implementation::AllocOncePointer<InverseBiMap>;

        friend class implementation::AllocOncePointer<BidirectionalMap>;

        static_assert(std::is_default_constructible<ForwardMap>::value,
                      "ForwardMap base containers must be default constructible.");
        static_assert(std::is_copy_constructible<ForwardMap>::value,
                      "ForwardMap base containers must be copy constructible.");

        explicit BidirectionalMap(
                InverseBiMap &inverseMap) noexcept(std::is_nothrow_default_constructible_v<ForwardMap>)
                : map(), inverseAccess(&inverseMap) {}

    public:
        class Iterator {
        public:
            using IteratorType = decltype(std::declval<ForwardMap>().cbegin());
            using ValueType = std::pair<const ForwardKey &, const InverseKey &>;

            Iterator() noexcept(std::is_nothrow_default_constructible_v<IteratorType>): it(), val(std::nullopt),
                                                                                        container(nullptr), end(true) {}

            Iterator(IteratorType it, const ForwardMap &container)
            noexcept(std::is_nothrow_copy_constructible_v<IteratorType> && noexcept(ValueType(it->first, *it->second)))
                    : it(it), container(&container),
                      end(this->it == std::end(container)) {
                if (!end) {
                    val.emplace(it->first, *it->second);
                }
            }

            explicit Iterator(const BidirectionalMap &map) noexcept(noexcept(Iterator(std::declval<IteratorType>(),
                    std::declval<BidirectionalMap>().map))) : Iterator(map.map.begin(), map.map) {}

            Iterator(const Iterator &other) = default;

            Iterator(Iterator &&other) noexcept = default;

            Iterator &operator=(Iterator other) noexcept(noexcept(ValueType(this->it->first, *this->it->second))) {
                it = std::move(other.it);
                val.reset();
                end = other.end;
                if (!end) {
                    val.emplace(*other.val);
                }

                return *this;
            }

            ~Iterator() = default;

            Iterator &operator++() noexcept(noexcept(++this->it) &&
                                            noexcept(ValueType(this->it->first, *this->it->second))) {
                if (end) {
                    return *this;
                }

                ++it;
                if (it != std::end(*container)) {
                    val.emplace(it->first, *it->second);
                } else {
                    val.reset();
                    end = true;
                }

                return *this;
            }

            bool operator==(const Iterator &other) const noexcept(noexcept(it == it)) {
                return (end && other.end) || (end == other.end && this->it == other.it);
            }

            bool operator!=(const Iterator &other) const noexcept(noexcept(*this == other)) {
                return !(*this == other);
            }

            ValueType operator*() const noexcept {
                return *val;
            }

            const ValueType *operator->() const noexcept {
                return &*val;
            }

        private:
            friend class BidirectionalMap;

            IteratorType it;
            std::optional<ValueType> val{};
            const ForwardMap *container;
            bool end;
        };

        /**
         * Creates an empty container
         */
        BidirectionalMap() : map(), inverseAccess(*this) {}

        /**
         * Creates the container from the iterator range [start, end)
         * @tparam InputIt Type of iterator
         * @param start
         * @param end
         */
        template<typename InputIt>
        BidirectionalMap(InputIt start, InputIt end) : BidirectionalMap() {
            while (start != end) {
                emplace(*start);
                ++start;
            }
        }

        /**
         * Creates the container from the given initializer list
         * @param init
         */
        BidirectionalMap(std::initializer_list<std::pair<ForwardKey, InverseKey>> init) :
                BidirectionalMap(init.begin(), init.end()) {}

        /**
         * Copy constructor
         * @param other
         */
        BidirectionalMap(const BidirectionalMap &other) : map(other.map), inverseAccess(*this) {
            inverseAccess->map = other.inverseAccess->map;
        }

        /**
         * Swaps the content of the containers. If ForwardMapType and InverseMapType support moving, no objects are
         * copied
         * @param other
         */
        void swap(BidirectionalMap &other) noexcept(std::is_nothrow_swappable_v<ForwardMap> &&
                                                    std::is_nothrow_swappable_v<typename InverseBiMap::ForwardMap>) {
            std::swap(this->map, other.map);
            std::swap(this->inverseAccess->map, other.inverseAccess->map);
        }


        /**
         * Move constructor. Moves objects from other. If ForwardMapType and InverseMapType support moving, no objects
         * are copied
         * @param other
         * @note unlike containers like std::unordered_map (at least using gcc), use after move will most likely result
         * in undefined behaviour
         */
        BidirectionalMap(BidirectionalMap &&other)
        noexcept(std::is_nothrow_default_constructible_v<ForwardMap> &&
                 std::is_nothrow_swappable_v<ForwardMap> &&
                 std::is_nothrow_swappable_v<typename InverseBiMap::ForwardMap>) : map(), inverseAccess() {
            std::swap(map, other.map);
            inverseAccess.swap(other.inverseAccess);
            inverseAccess->inverseAccess = this;
        }

        BidirectionalMap &operator=(BidirectionalMap other) noexcept(noexcept(this->swap(other))) {
            swap(other);
            return *this;
        }

        ~BidirectionalMap() = default;

        /**
         * Constructs elements in place. If a pair of values with same ForwardKey or same InverseKey already exists
         * no insertion happens
         * @tparam ARGS
         * @param args
         * @return std::pair(Iterator to inserted element or already existing element, bool whether insertion happened)
         */
        template<typename ...ARGS>
        auto emplace(ARGS &&...args) -> std::pair<Iterator, bool> {
            std::pair<ForwardKey, InverseKey> tmp(std::forward<ARGS>(args)...);
            auto res = find(tmp.first);
            if (res != end()) {
                return {res, false};
            }

            auto invRes = invert().find(tmp.second);
            if (invRes != invert().end()) {
                return {find(invRes->second), false};
            }

            auto it = map.emplace(std::move(tmp.first), nullptr).first;
            auto invIt = inverseAccess->map.emplace(std::move(tmp.second), &it->first).first;
            it->second = &invIt->first;
            return {Iterator(it, map), true};
        }

        /**
         * Number of contained elements
         * @return
         */
        [[nodiscard]] auto size() const noexcept(noexcept(map.size())) {
            return map.size();
        }

        /**
         * Whether container is empty
         * @return
         */
        [[nodiscard]] bool empty() const noexcept(noexcept(map.empty())) {
            return map.empty();
        }

        /**
         * Calls reserve on underlying container
         * @param n Number of elements to reserve space for
         */
        void reserve(std::size_t n) noexcept(noexcept(map.reserve(n)) && noexcept(inverseAccess->map.reserve(n))) {
            map.reserve(n);
            inverseAccess->map.reserve(n);
        }

        /**
         * Access to the inverted map for reverse lookup or insertion
         * @return Reference to inverted map
         */
        auto invert() noexcept -> InverseBiMap & {
            return *inverseAccess;
        }

        /**
         * Readonly access to the inverted map for reverse lookup
         * @return const reference to inverted map
         */
        auto invert() const noexcept -> const InverseBiMap & {
            return *inverseAccess;
        }

        /**
         * Iterator to first element
         * @note Ordering of objects depends on the underlying container specified by ForwardMpaType and InverseMapType.
         * Ordering of forward access may be different from ordering of inverse access
         * @return
         */
        Iterator begin() const noexcept(noexcept(Iterator(*this))) {
            return Iterator(*this);
        }

        /**
         * Iterator to the past the end element. This iterator does not point to anything. Access results in undefined
         * behaviour
         * @return
         */
        constexpr Iterator end() const noexcept(noexcept(Iterator())) {
            return Iterator();
        }

        /**
         * Finds an element with forward key equivalent to key
         * @param key
         * @return Iterator to an element with forward key equivalent to key. If no such element is found, past-the-end
         * (see end()) iterator is returned.
         */
        Iterator find(const ForwardKey &key) const {
            return Iterator(map.find(key), map);
        }

        /**
         * Erases the element at position pos
         * @param pos Iterator to the element to remove. if pos == end(), this method does nothing
         * @return Iterator pointing to the next element in the container
         */
        Iterator erase(Iterator pos) {
            if (pos == end()) {
                return pos;
            }

            inverseAccess->map.erase(inverseAccess->map.find(pos->second));
            return Iterator(map.erase(pos.it), map);
        }

        /**
         * Erases an element with forward key equivalent to key.
         * @param key
         * @return 1 if an element with forward key is found, 0 otherwise
         */
        std::size_t erase(const ForwardKey &key) {
            auto it = Iterator(map.find(key), map);
            if (it != end()) {
                erase(it);
                return 1;
            }

            return 0;
        }

        /**
         * Erases all elements in the range [first, last) which must be a valid range in *this
         * @param first
         * @param last
         * @return Iterator following the last removed element
         */
        Iterator erase(Iterator first, Iterator last) {
            while (first != last && first != end()) {
                first = erase(first);
            }

            return first;
        }

        /**
         * Compares containers by elements
         * @param other
         * @return true if other contains the same elements and the same number of elements. Two elements are equivalent
         * if their ForwardKeys and InverseKeys are equivalent respectively. Otherwise false is returned
         */
        bool operator==(const BidirectionalMap &other) const {
            if (size() != other.size()) {
                return false;
            }

            for (const auto &[v1, v2] : *this) {
                auto res = other.find(v1);
                if (res == other.end() || res->second != v2) {
                    return false;
                }
            }

            return true;
        }

        /**
         * Compares container by elements, see operator==
         * @param other
         * @return
         */
        bool operator!=(const BidirectionalMap &other) const noexcept(noexcept(*this == other)) {
            return !(*this == other);
        }

        /**
         * Erases all elements from the container
         */
        void clear() noexcept(noexcept(map.clear()) && noexcept(inverseAccess->map.clear())) {
            map.clear();
            inverseAccess->map.clear();
        }

    private:
        ForwardMap map;
        InversBiMapPtr inverseAccess;
    };
}

namespace std {
    /**
     * Specialization to std::swap
     * @tparam ForwardKey
     * @tparam InverseKey
     * @tparam ForwardMapType
     * @tparam InverseMapType
     * @param lhs
     * @param rhs
     */
    template<typename ForwardKey, typename InverseKey,
            template<typename T, typename U> typename ForwardMapType = std::unordered_map,
            template<typename T, typename U> typename InverseMapType = std::unordered_map>
    void swap(BiMap::BidirectionalMap<ForwardKey, InverseKey, ForwardMapType, InverseMapType> &lhs,
              BiMap::BidirectionalMap<ForwardKey, InverseKey, ForwardMapType, InverseMapType> &rhs)
              noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }
}

#endif //BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
