/**
 * @author Tim Luchterhand
 * @date 2021-06-16
 * @brief This file contains the class definition of a bidirectional associative container that can be used for
 * efficient lookup in both directions. Its contents are immutable to ensure the integrity of the underlying map
 * map containers. Also the mapping has to be injective to make bidirectional lookup possible.
 */

#ifndef BIDIRECTIONALMAP_BIDIRECTIONAL_MAP_HPP
#define BIDIRECTIONALMAP_BIDIRECTIONAL_MAP_HPP

#include <unordered_map>
#include <optional>

namespace bimap::implementation {
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
        constexpr AllocOncePointer(T *data) noexcept: data(data), owner(false) {}

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
        bool operator!=(const Ptr &other) const {
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

namespace bimap {
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
    class bidirectional_map {
    private:
        using ForwardMap = ForwardMapType<ForwardKey, const InverseKey *>;
        using InverseBiMap = bidirectional_map<InverseKey, ForwardKey, InverseMapType, ForwardMapType>;
        friend InverseBiMap;
        using InversBiMapPtr = implementation::AllocOncePointer<InverseBiMap>;

        friend class implementation::AllocOncePointer<bidirectional_map>;

        static_assert(std::is_default_constructible<ForwardMap>::value,
                      "ForwardMap base containers must be default constructible.");
        static_assert(std::is_copy_constructible<ForwardMap>::value,
                      "ForwardMap base containers must be copy constructible.");

        explicit bidirectional_map(
                InverseBiMap &inverseMap) noexcept(std::is_nothrow_default_constructible_v<ForwardMap>)
                : map(), inverseAccess(&inverseMap) {}

    public:
        class iterator {
            using IteratorType = decltype(std::declval<std::add_const_t<ForwardMap>>().begin());

            friend class bidirectional_map;

        public:
            using value_type = std::pair<const ForwardKey &, const InverseKey &>;
            using reference = value_type;
            using pointer = std::add_pointer_t<std::add_const_t<value_type>>;
            using diference_type = typename std::iterator_traits<IteratorType>::difference_type;
            using iterator_category = typename std::iterator_traits<IteratorType>::iterator_category;

            /**
             * CTor used for creating end iterator
             * @param container
             */
            explicit iterator(const ForwardMap &container) noexcept(noexcept(container.end()) &&
                                                                    std::is_nothrow_copy_constructible_v<IteratorType>)
                    : it(container.end()), sentinel(container.end()),
                      val(std::nullopt) {}

            /**
             * CTor
             * @param it iterator to underlying map element
             * @param container corresponding container used for range checking
             */
            iterator(IteratorType it, const ForwardMap &container)
            noexcept(std::is_nothrow_copy_constructible_v<IteratorType> && noexcept(value_type(it->first, *it->second)))
                    : it(it), sentinel(std::end(container)) {
                if (it != sentinel) {
                    val.emplace(it->first, *it->second);
                }
            }

            iterator(const iterator &) = default;

            iterator(iterator &&) noexcept = default;

            ~iterator() = default;

            /**
             * Assignment operator
             * @param other
             * @return
             */
            iterator &operator=(iterator other) {
                it = other.it;
                sentinel = other.sentinel;
                if (it != sentinel) {
                    val.emplace(it->first, *it->second);
                }

                return *this;
            }

            /**
             * Increments underlying iterator by one
             * @return
             */
            iterator &operator++() {
                ++it;
                if (it != sentinel) {
                    val.emplace(it->first, *it->second);
                }

                return *this;
            }

            /**
             * Post increment. Increments underlying iterator by one
             * @return
             */
            iterator operator++(int) {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            /**
             * Equality operator. Compares underlying map iterators
             * @param other
             * @return
             */
            constexpr bool operator==(const iterator &other) const noexcept(noexcept(std::declval<IteratorType>() ==
                                                                                     std::declval<IteratorType>())) {
                return it == other.it;
            }

            /**
             * Inequality operator. Compares underlying map iterators
             * @param other
             * @return
             */
            constexpr bool operator!=(const iterator &other) const noexcept(noexcept(std::declval<iterator>() ==
                                                                                     std::declval<iterator>())) {
                return !(*this == other);
            }

            /**
             * Returns a pair of reference to container elements
             * @return
             */
            constexpr value_type operator*() const noexcept {
                return *val;
            }

            constexpr pointer operator->() const noexcept {
                return &*val;
            }

        private:
            IteratorType it;
            IteratorType sentinel;
            std::optional<value_type> val{};
        };

        /**
         * Creates an empty container
         */
        bidirectional_map() : map(), inverseAccess(*this) {}

        /**
         * Creates the container from the iterator range [start, end)
         * @tparam InputIt Type of iterator
         * @param start
         * @param end
         */
        template<typename InputIt>
        bidirectional_map(InputIt start, InputIt end) : bidirectional_map() {
            while (start != end) {
                emplace(*start);
                ++start;
            }
        }

        /**
         * Creates the container from the given initializer list
         * @param init
         */
        bidirectional_map(std::initializer_list<std::pair<ForwardKey, InverseKey>> init) :
                bidirectional_map(init.begin(), init.end()) {}

        /**
         * Copy constructor
         * @param other
         */
        bidirectional_map(const bidirectional_map &other) : bidirectional_map() {
            for (const auto &valuePair: other) {
                emplace(valuePair);
            }
        }

        /**
         * Swaps the content of the containers. If ForwardMapType and InverseMapType support moving, no objects are
         * copied
         * @param other
         */
        void swap(bidirectional_map &other) noexcept(std::is_nothrow_swappable_v<ForwardMap> &&
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
        bidirectional_map(bidirectional_map &&other)
        noexcept(std::is_nothrow_default_constructible_v<ForwardMap> &&
                 std::is_nothrow_swappable_v<ForwardMap> &&
                 std::is_nothrow_swappable_v<typename InverseBiMap::ForwardMap>): map(), inverseAccess() {
            std::swap(map, other.map);
            inverseAccess.swap(other.inverseAccess);
            inverseAccess->inverseAccess = this;
        }

        bidirectional_map &operator=(bidirectional_map other) noexcept(noexcept(this->swap(other))) {
            swap(other);
            return *this;
        }

        ~bidirectional_map() = default;

        /**
         * Constructs elements in place. If a pair of values with same ForwardKey or same InverseKey already exists
         * no insertion happens
         * @tparam ARGS
         * @param args
         * @return std::pair(iterator to inserted element or already existing element, bool whether insertion happened)
         */
        template<typename ...ARGS>
        auto emplace(ARGS &&...args) -> std::pair<iterator, bool> {
            std::pair<ForwardKey, InverseKey> tmp(std::forward<ARGS>(args)...);
            auto res = find(tmp.first);
            if (res != end()) {
                return {res, false};
            }

            auto invRes = inverse().find(tmp.second);
            if (invRes != inverse().end()) {
                return {find(invRes->second), false};
            }

            auto it = map.emplace(std::move(tmp.first), nullptr).first;
            auto invIt = inverseAccess->map.emplace(std::move(tmp.second), &it->first).first;
            it->second = &invIt->first;
            return {iterator(it, map), true};
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
        auto inverse() noexcept -> InverseBiMap & {
            return *inverseAccess;
        }

        /**
         * Readonly access to the inverted map for reverse lookup
         * @return const reference to inverted map
         */
        auto inverse() const noexcept -> const InverseBiMap & {
            return *inverseAccess;
        }

        /**
         * iterator to first element
         * @note Ordering of objects depends on the underlying container specified by ForwardMpaType and InverseMapType.
         * Ordering of forward access may be different from ordering of inverse access
         * @return
         */
        iterator begin() const {
            return iterator(std::begin(map), map);
        }

        /**
         * iterator to the past the end element. This iterator does not point to anything. Access results in undefined
         * behaviour
         * @return
         */
        constexpr iterator end() const {
            return iterator(map);
        }

        /**
         * Finds an element with forward key equivalent to key
         * @param key
         * @return iterator to an element with forward key equivalent to key. If no such element is found, past-the-end
         * (see end()) iterator is returned.
         */
        iterator find(const ForwardKey &key) const {
            return iterator(map.find(key), map);
        }

        /**
         * Erases the element at position pos
         * @param pos iterator to the element to remove. if pos == end(), this method does nothing
         * @return iterator pointing to the next element in the container
         */
        iterator erase(iterator pos) {
            if (pos == end()) {
                return pos;
            }

            inverseAccess->map.erase(inverseAccess->map.find(pos->second));
            return iterator(map.erase(pos.it), map);
        }

        /**
         * Erases an element with forward key equivalent to key.
         * @param key
         * @return 1 if an element with forward key is found, 0 otherwise
         */
        std::size_t erase(const ForwardKey &key) {
            auto it = iterator(map.find(key), map);
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
         * @return iterator following the last removed element
         */
        iterator erase(iterator first, iterator last) {
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
        bool operator==(const bidirectional_map &other) const {
            if (size() != other.size()) {
                return false;
            }

            for (const auto &[v1, v2]: *this) {
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
        bool operator!=(const bidirectional_map &other) const noexcept(noexcept(*this == other)) {
            return !(*this == other);
        }

        /**
         * Erases all elements from the container
         */
        void clear() noexcept(noexcept(map.clear()) && noexcept(inverseAccess->map.clear())) {
            map.clear();
            inverseAccess->map.clear();
        }

        /**
         * Check if a certain key can be found
         * @param key
         * @return true if key can be found, false otherwise
         */
        bool contains(const ForwardKey &key) const {
            return find(key) != end();
        }

        auto at(const ForwardKey &key) const -> const InverseKey & {
            auto res = find(key);
            if (res == end()) {
                throw std::out_of_range("Bimap key not found");
            }

            return res->second;
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
    void swap(bimap::bidirectional_map <ForwardKey, InverseKey, ForwardMapType, InverseMapType> &lhs,
              bimap::bidirectional_map <ForwardKey, InverseKey, ForwardMapType, InverseMapType> &rhs)
    noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }
}

#endif //BIDIRECTIONALMAP_BIDIRECTIONAL_MAP_HPP
