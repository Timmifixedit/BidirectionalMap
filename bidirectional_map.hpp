/**
 * @author Tim Luchterhand
 * @date 2021-06-16
 * @brief This file contains the class definition of a bidirectional associative container that can be used for
 * efficient lookup in both directions. Its contents are immutable to ensure the integrity of the underlying map
 * map containers.
 */

#ifndef BIDIRECTIONALMAP_BIDIRECTIONAL_MAP_HPP
#define BIDIRECTIONALMAP_BIDIRECTIONAL_MAP_HPP

#include <unordered_map>
#include <map>
#include <optional>
#include <stdexcept>
#include <cassert>

namespace bimap::impl {
    namespace traits {
        template<typename T, typename = std::void_t<>>
        struct is_bidirectional {
            static constexpr bool value = false;
        };

        template<typename T>
        struct is_bidirectional<T, std::void_t<typename std::iterator_traits<T>::iterator_category>> {
            static constexpr bool value = std::is_base_of_v<std::bidirectional_iterator_tag,
                    typename std::iterator_traits<T>::iterator_category>;
        };

        template<typename T>
        constexpr inline bool is_bidirectional_v = is_bidirectional<T>::value;

        template<typename T>
        struct get_first {
            explicit constexpr get_first(const T &value) noexcept: value(value) {}
            const T &value;
        };

        template<typename T, typename U>
        struct get_first<std::pair<T, U>> {
            explicit constexpr get_first(const std::pair<T, U> &pair) noexcept: value(pair.first) {}
            const T &value;
        };

        template<typename T>
        struct is_multimap {
            static constexpr bool value = false;
        };

        template<typename Key, typename Val, typename Comp, typename Alloc>
        struct is_multimap<std::multimap<Key, Val, Comp, Alloc>> {
            static constexpr bool value = true;
        };

        template<typename Key, typename Val, typename Hash, typename Comp, typename Alloc>
        struct is_multimap<std::unordered_multimap<Key, Val, Hash, Comp, Alloc>> {
            static constexpr bool value = true;
        };

        template<typename T>
        struct is_ordered {
            static constexpr bool value = false;
        };

        template<typename Key, typename Val, typename Comp, typename Alloc>
        struct is_ordered<std::multimap<Key, Val, Comp, Alloc>> {
            static constexpr bool value = true;
        };

        template<typename Key, typename Val, typename Comp, typename Alloc>
        struct is_ordered<std::map<Key, Val, Comp, Alloc>> {
            static constexpr bool value = true;
        };

        template<typename T>
        constexpr inline bool is_multimap_v = is_multimap<T>::value;

        template<typename T>
        constexpr inline bool nothrow_comparable = noexcept(std::declval<T>() == std::declval<T>());

        template<typename T>
        constexpr inline bool is_ordered_v = is_ordered<T>::value;
    }

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
        constexpr AllocOncePointer(const AllocOncePointer &other) noexcept: data(other.data), owner(false) {}

        /**
         * Swaps pointer and ownership with other
         * @param other
         */
        constexpr void swap(AllocOncePointer &other) noexcept {
            std::swap(data, other.data);
            std::swap(owner, other.owner);
        }

        constexpr AllocOncePointer(AllocOncePointer &&other) noexcept: AllocOncePointer() {
            swap(other);
        }

        constexpr AllocOncePointer &operator=(AllocOncePointer other) noexcept {
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
        [[nodiscard]] constexpr bool isOwner() const noexcept {
            return owner;
        }

        constexpr T &operator*() noexcept {
            return *data;
        }

        constexpr const T &operator*() const noexcept {
            return *data;
        }

        constexpr T *operator->() noexcept {
            return data;
        }

        constexpr const T *operator->() const noexcept {
            return data;
        }

        /**
         * Comparison operator
         * @param other
         * @return true if data pointers point to the same object, false otherwise
         */
        constexpr bool operator==(const AllocOncePointer &other) const noexcept {
            return data == other.data;
        }

        constexpr bool operator==(std::nullptr_t) const noexcept {
            return data == nullptr;
        }

        constexpr bool operator!=(std::nullptr_t) const noexcept {
            return data != nullptr;
        }

        constexpr bool operator!=(const T *other) const noexcept {
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
    constexpr void swap(AllocOncePointer<T> &a, AllocOncePointer<T> &b) noexcept {
        a.swap(b);
    }

    /**
     * Non owning pointer to an object. It overloads the equality operators in order to compare the underlying objects
     * instead of the pointer values
     * @tparam T
     */
    template<typename T>
    class Surrogate {
    public:
        constexpr Surrogate(T *data) noexcept: data(data) {}

        /**
         * Compares objects behind the pointer
         * @param other
         * @return
         */
        constexpr bool operator==(Surrogate other) const noexcept(traits::nothrow_comparable<T>) {
            return *data == *other.data;
        }

        /**
         * Compares objects behind the pointer
         * @param other
         * @return
         */
        constexpr bool operator!=(Surrogate other) const noexcept(noexcept(other == other)) {
            return !(*this == other);
        }

        constexpr T& operator*() noexcept {
            return *data;
        }

        constexpr const T& operator*() const noexcept {
            return *data;
        }

        constexpr T* operator->() noexcept {
            return data;
        }

        constexpr const T* operator->() const noexcept {
            return data;
        }

        constexpr T* get() noexcept {
            return data;
        }

        constexpr const T* get() const noexcept {
            return data;
        }

    private:
        T * data;
    };

}

namespace bimap {
    /**
     * Bidirectional associative container that supports efficient lookup in both directions. Neither items of type
     * ForwardKey nor InverseKey can be modified.
     * @tparam ForwardKey Type of key used for forward lookup
     * @tparam InverseKey Type of key used for inverse lookup
     * @tparam ForwardMapType base map container used for forward lookup. Default is std::unordered_map
     * @tparam InverseMapType base map container used for inverse lookup. Default is std::unordered_map
     */
    template<typename ForwardKey, typename InverseKey,
             template<typename ...> typename ForwardMapType = std::unordered_map,
             template<typename ...> typename InverseMapType = std::unordered_map>
    class bidirectional_map {
    private:
        using ForwardMap = ForwardMapType<ForwardKey, impl::Surrogate<const InverseKey>>;
        using InverseBiMap = bidirectional_map<InverseKey, ForwardKey, InverseMapType, ForwardMapType>;
        friend InverseBiMap;
        using InverseMap = typename InverseBiMap::ForwardMap;
        using InversBiMapPtr = impl::AllocOncePointer<InverseBiMap>;

        friend class impl::AllocOncePointer<bidirectional_map>;

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
            static constexpr bool copy_constructible = std::is_nothrow_copy_constructible_v<IteratorType>;
            static constexpr bool copy_assignable = std::is_nothrow_copy_assignable_v<IteratorType>;

            friend class bidirectional_map;
            friend InverseBiMap;

        public:
            using value_type = std::pair<const ForwardKey &, const InverseKey &>;
            using reference = value_type;
            using pointer = std::add_pointer_t<std::add_const_t<value_type>>;
            using diference_type = typename std::iterator_traits<IteratorType>::difference_type;
            using iterator_category = typename std::iterator_traits<IteratorType>::iterator_category;

            /**
             * CTor
             * @param it iterator to underlying map element
             */
            constexpr explicit iterator(const IteratorType &it) noexcept(copy_constructible): it(it) {}

            constexpr iterator(const iterator &other) noexcept(std::is_constructible_v<iterator, IteratorType>)
                    : iterator(other.it) {}

            constexpr iterator(iterator &&other) noexcept(std::is_constructible_v<iterator, IteratorType>): iterator(
                    other) {}

            constexpr iterator &operator=(iterator other) noexcept(copy_assignable) {
                it = other.it;
                return *this;
            }

            ~iterator() = default;

            /**
             * Increments underlying iterator by one
             * @return
             */
            constexpr iterator &operator++() noexcept(noexcept(++std::declval<IteratorType>())) {
                ++it;
                return *this;
            }

            /**
             * Post increment. Increments underlying iterator by one
             * @return
             */
            constexpr iterator operator++(int) noexcept(std::is_nothrow_copy_constructible_v<iterator> &&
                                                        noexcept(++std::declval<iterator>())) {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            /**
             * Decrements underlying iterator by one. Only available if base iterator supports bidirectional iteration
             * @tparam IsBidirectional
             * @return
             */
            template<bool IsBidirectional = impl::traits::is_bidirectional_v<IteratorType>>
            constexpr auto operator--() noexcept(noexcept(--std::declval<IteratorType>()))
            -> std::enable_if_t<IsBidirectional, iterator &> {
                --it;
                return *this;
            }

            /**
             * Post decrement. Only available if base iterator supports bidirectional iteration
             * @tparam IsBidirectional
             * @return
             */
            template<bool IsBidirectional = impl::traits::is_bidirectional_v<IteratorType>>
            constexpr auto operator--(int) noexcept(std::is_nothrow_copy_constructible_v<iterator> &&
                                                    noexcept(--std::declval<iterator>()))
                                                    -> std::enable_if_t<IsBidirectional, iterator> {
                auto tmp = *this;
                --*this;
                return tmp;
            }

            /**
             * Equality operator. Compares underlying map iterators
             * @param other
             * @return
             */
            constexpr bool operator==(const iterator &other) const
                noexcept(impl::traits::nothrow_comparable<IteratorType>) {
                return it == other.it;
            }

            /**
             * Inequality operator. Compares underlying map iterators
             * @param other
             * @return
             */
            constexpr bool operator!=(const iterator &other) const noexcept(noexcept(other == other)) {
                return !(*this == other);
            }

            /**
             * Returns a pair of reference to container elements
             * @return
             */
            constexpr reference operator*() const {
                return reference(it->first, *it->second);
            }

            constexpr pointer operator->() const {
                val.emplace(it->first, *it->second);
                return &*val;
            }

        private:
            IteratorType it;
            mutable std::optional<value_type> val{};
        };

    private:
        using BaseIterator = decltype(std::declval<ForwardMap>().begin());
        static constexpr bool iterator_ctor_nothrow = std::is_nothrow_constructible_v<iterator, BaseIterator>;
    public:
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
                                                     std::is_nothrow_swappable_v<InverseMap>) {
            std::swap(this->map, other.map);
            std::swap(this->inverseAccess->map, other.inverseAccess->map);
        }

        /**
         * Move constructor. Moves objects from other. If ForwardMapType and InverseMapType support moving, no objects
         * are copied
         * @param other
         */
        bidirectional_map(bidirectional_map &&other) : bidirectional_map() {
            swap(other);
        }

        bidirectional_map &operator=(bidirectional_map other)
                noexcept(noexcept(std::declval<bidirectional_map>().swap(other))) {
            swap(other);
            return *this;
        }

        ~bidirectional_map() = default;

        /**
         * Constructs elements in place. If a pair of values with same ForwardKey or same InverseKey already exists
         * and the corresponding container requires unique keys, then no insertion happens.
         * @tparam ARGS
         * @param args arguments used to construct elements
         * @return std::pair(iterator to inserted element or already existing element, bool whether insertion happened)
         * @example if std::multiset is used for forward lookup and the map contains the following pair :(a, b)
         * then inserting (a, b') is possible whereas (a', b) will not be inserted since the inverse lookup is carried
         * out by std::unordered_map
         */
        template<typename ...ARGS>
        auto emplace(ARGS &&...args) -> std::pair<iterator, bool> {
            std::pair<ForwardKey, InverseKey> tmp(std::forward<ARGS>(args)...);
            if constexpr(!impl::traits::is_multimap_v<ForwardMap>) {
                auto res = find(tmp.first);
                if (res != end()) {
                    return {res, false};
                }
            }

            if constexpr(!impl::traits::is_multimap_v<InverseMap>) {
                auto invRes = inverse().find(tmp.second);
                if (invRes != inverse().end()) {
                    return {find(invRes->second), false};
                }
            }

            auto it = impl::traits::get_first(map.emplace(std::move(tmp.first), nullptr)).value;
            auto invIt = impl::traits::get_first(inverseAccess->map.emplace(std::move(tmp.second), &it->first)).value;
            it->second = &invIt->first;
            return {iterator(it), true};
        }

        /**
         * Number of contained elements
         * @return
         */
        [[nodiscard]] auto size() const noexcept(noexcept(std::declval<ForwardMap>().size())) {
            return map.size();
        }

        /**
         * Whether container is empty
         * @return
         */
        [[nodiscard]] bool empty() const noexcept(noexcept(std::declval<ForwardMap>().empty())) {
            return map.empty();
        }

        /**
         * Access to the inverted map for reverse lookup or insertion
         * @return Reference to inverted map
         */
        constexpr auto inverse() noexcept -> InverseBiMap & {
            return *inverseAccess;
        }

        /**
         * Readonly access to the inverted map for reverse lookup
         * @return const reference to inverted map
         */
        constexpr auto inverse() const noexcept -> const InverseBiMap & {
            return *inverseAccess;
        }

        /**
         * iterator to first element
         * @note Ordering of objects depends on the underlying container specified by ForwardMpaType and InverseMapType.
         * Ordering of forward access may be different from ordering of inverse access
         * @return
         */
        iterator begin() const noexcept(noexcept(std::declval<ForwardMap>().begin()) && iterator_ctor_nothrow) {
            return iterator(map.begin());
        }

        /**
         * iterator to the past the end element. This iterator does not point to anything. Access results in undefined
         * behaviour
         * @return
         */
        iterator end() const noexcept(noexcept(std::declval<ForwardMap>().end()) && iterator_ctor_nothrow) {
            return iterator(map.end());
        }

        /**
         * Finds an element with forward key equivalent to key
         * @param key
         * @return iterator to an element with forward key equivalent to key. If no such element is found, past-the-end
         * (see end()) iterator is returned.
         */
        iterator find(const ForwardKey &key) const noexcept(noexcept(std::declval<ForwardMap>().find(key)) &&
                                                            iterator_ctor_nothrow) {
            return iterator(map.find(key));
        }


        /**
         * Calls lower_bound on the underlying container. For more information see documentation of the respective
         * container type. Only available when using sorted containers like std::map
         * @tparam IsOrdered
         * @param key Key used for lookup
         * @return lower bound iterator
         */
        template<bool IsOrdered = impl::traits::is_ordered_v<ForwardMap>>
        auto lower_bound(const ForwardKey &key) const noexcept(noexcept(std::declval<ForwardMap>().lower_bound(key)) &&
                                                               iterator_ctor_nothrow)
                                                               -> std::enable_if_t<IsOrdered, iterator> {
            return iterator(map.lower_bound(key));
        }

        /**
         * Calls upper_bound on the underlying container. For more information see documentation of the respective
         * container type. Only available when using sorted containers like std::map
         * @tparam IsOrdered
         * @param key Key used for lookup
         * @return upper bound iterator
         */
        template<bool IsOrdered = impl::traits::is_ordered_v<ForwardMap>>
        auto upper_bound(const ForwardKey &key) const noexcept(noexcept(std::declval<ForwardMap>().upper_bound(key)) &&
                                                               iterator_ctor_nothrow)
                                                               -> std::enable_if_t<IsOrdered, iterator> {
            return iterator(map.upper_bound(key));
        }

        /**
         * Calls equal_range on the underlying container. For more information see documentation of the respective
         * container type.
         * @param key Key used for lookup
         * @return iterator range containing equal elements
         */
        auto equal_range(const ForwardKey &key) const noexcept(noexcept(std::declval<ForwardMap>().equal_range(key)) &&
                                                               iterator_ctor_nothrow) -> std::pair<iterator, iterator> {
            auto [first, last] = map.equal_range(key);
            return {iterator(first), iterator(last)};
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

            if constexpr(impl::traits::is_multimap_v<InverseMap>) {
                auto [curr, end] = inverse().equal_range(pos->second);
                while (curr != end && &curr->first != pos.it->second.get()) {
                    ++curr;
                }

                assert(curr != end);
                inverseAccess->map.erase(curr.it);

            } else {
                inverseAccess->map.erase(inverseAccess->map.find(pos->second));
            }

            return iterator(map.erase(pos.it));
        }

        /**
         * Erases all elements with forward key equivalent to key.
         * @param key
         * @return number of erased elements
         */
        std::size_t erase(const ForwardKey &key) {
            auto [curr, last] = equal_range(key);
            std::size_t numErased = 0;
            while (curr != last) {
                curr = erase(curr);
                ++numErased;
            }

            return numErased;
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
         * Compares underlying containers
         * @param other
         * @return true if both forward mapping and inverse mapping are equivalent
         * @note for more details see documentation of the used underlying containers. If the default containers are
         * used, the underlying std::unordered_maps are compared
         */
        bool operator==(const bidirectional_map &other) const noexcept(impl::traits::nothrow_comparable<ForwardMap> &&
                                                                       impl::traits::nothrow_comparable<InverseMap>) {
            return map == other.map && inverseAccess->map == other.inverseAccess->map;
        }

        /**
         * Compares container by elements, see operator==
         * @param other
         * @return
         */
        bool operator!=(const bidirectional_map &other) const noexcept(noexcept(other == other)) {
            return !(*this == other);
        }

        /**
         * Erases all elements from the container
         */
        void clear() noexcept(noexcept(std::declval<ForwardMap>().clear()) &&
                              noexcept(std::declval<InverseMap>().clear())) {
            map.clear();
            inverseAccess->map.clear();
        }

        /**
         * Check if a certain key can be found
         * @param key
         * @return true if key can be found, false otherwise
         */
        bool contains(const ForwardKey &key) const noexcept(noexcept(std::declval<bidirectional_map>().find(key)) &&
                                                            noexcept(std::declval<iterator>() !=
                                                                     std::declval<iterator>())) {
            return find(key) != end();
        }

        /**
         * Returns the value found by the given key
         * @param key
         * @return reference to found value
         * @throws out_of_range if ey does not exist
         */
        template<bool UniqueKeys = !impl::traits::is_multimap_v<ForwardMap>>
        auto at(const ForwardKey &key) const -> std::enable_if_t<UniqueKeys, const InverseKey &> {
            auto res = find(key);
            if (res == end()) {
                throw std::out_of_range("bidirectional map key not found");
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
