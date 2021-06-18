//
// Created by tim on 16.06.21.
//

#ifndef BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
#define BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
#include <unordered_map>
#include <memory>

namespace BiMap {

    template<typename T, typename U>
    class BidirectionalMap;

    template<typename T, typename U>
    class BiMapImpl {
        using ForwardKey = T;
        using InverseKey = U;
        using ForwardMap = std::unordered_map<T, const U*>;
        using InverseMap = std::unordered_map<U, const T*>;
        using ForwardMapPtr = std::shared_ptr<ForwardMap>;
        using InverseMapPtr = std::shared_ptr<InverseMap>;
        using InverseBiMap = BiMapImpl<InverseKey, ForwardKey>;
        using InverseBiMapPtr = std::shared_ptr<InverseBiMap>;
        friend InverseBiMap;
        friend class BidirectionalMap<T, U>;
        friend class BidirectionalMap<U, T>;

        enum class Construct {
            Empty
        };

        explicit constexpr BiMapImpl(Construct) : forward(nullptr), inverse(nullptr), inverseAccess(nullptr) {}

        static void swap(BiMapImpl &m1, BiMapImpl &m2) {
            std::swap(m1.forward, m2.forward);
            std::swap(m1.inverse, m2.inverse);
        }

        BiMapImpl() : forward(std::make_shared<ForwardMap>()), inverse(std::make_shared<InverseMap>()),
            inverseAccess(nullptr) {}

        BiMapImpl(ForwardMapPtr f, InverseMapPtr i, InverseBiMapPtr invAcc) : forward(std::move(f)),
            inverse(std::move(i)), inverseAccess(invAcc) {}

    public:
        class Iterator {
        public:
            using IteratorType = decltype(std::declval<ForwardMap>().cbegin());
            using ValueType = std::pair<const ForwardKey&, const InverseKey&>;
            friend class BiMapImpl;

            Iterator() = default;

            explicit Iterator(IteratorType it) : it(it),
                val(this->it == IteratorType() ? nullptr : std::make_shared<ValueType>(it->first, *it->second)) {}

            explicit Iterator(const BiMapImpl &map) : Iterator(map.forward->begin()) {}

            Iterator &operator++() {
                ++it;
                if (it != IteratorType()) {
                    val = std::make_shared<ValueType>(it->first, *it->second);
                } else {
                    val = nullptr;
                }

                return *this;
            }

            bool operator==(const Iterator &other) const noexcept(noexcept(it == it)) {
                return this->it == other.it;
            }

            bool operator!=(const Iterator &other) const noexcept(noexcept(*this == other)) {
                return !(*this == other);
            }

            ValueType operator*() const  {
                return *val;
            }

            auto operator->() const noexcept {
                return val.get();
            }

        private:
            //friend class BiMapImpl;
            IteratorType it;
            std::shared_ptr<ValueType> val;
        };

        BiMapImpl(const BiMapImpl &other) = delete;

        BiMapImpl(BiMapImpl &&other) noexcept = delete;

        BiMapImpl &operator=(BiMapImpl &&other) = delete;

        BiMapImpl &operator=(const BiMapImpl &other) = delete;

        ~BiMapImpl() = default;

        template<typename ...ARGS>
        auto emplace(ARGS &&...args) -> std::pair<Iterator, bool> {
            std::pair<ForwardKey, InverseKey> tmp(std::forward<ARGS>(args)...);
            auto [it, inserted] = forward->emplace(std::move(tmp.first), nullptr);
            decltype(inverse->begin()) invIt;
            if (inserted) {
                std::tie(invIt, inserted) = inverse->emplace(std::move(tmp.second), &it->first);
            }

            if (inserted) {
                it->second = &invIt->first;
            }

            return {Iterator(it), inserted};
        }

        auto size() const noexcept(noexcept(forward->size())) {
            return forward->size();
        }

        bool empty() const noexcept(noexcept(forward->empty())) {
            return forward->empty();
        }

        void reserve(std::size_t n) noexcept(noexcept(forward->reserve(n)) && noexcept(inverse->reserve(n))) {
            forward->reserve(n);
            inverse->reserve(n);
        }

        auto invert() -> BiMapImpl<InverseKey, ForwardKey> & {
            return *inverseAccess;
        }

        auto invert() const -> const BiMapImpl<InverseKey, ForwardKey> & {
            return *inverseAccess;
        }

        Iterator begin() const noexcept(noexcept(Iterator(*this))) {
            return Iterator(*this);
        }

        Iterator end() const noexcept(noexcept(Iterator())) {
            return Iterator();
        }

        Iterator find(const ForwardKey &key) const {
            return Iterator(forward->find(key));
        }

        Iterator erase(Iterator pos) {
            if (pos == end()) {
                return pos;
            }

            inverse->erase(inverse->find(pos->second));
            return Iterator(forward->erase(pos.it));
        }

        std::size_t erase(const ForwardKey &key) {
            auto it = Iterator(forward->find(key));
            if (it != end()) {
                erase(it);
                return 1;
            }

            return 0;
        }

        Iterator erase(Iterator first, Iterator last) {
            while (first != last && first != end()) {
                first = erase(first);
            }

            return first;
        }

        void clear() {
            forward->clear();
            inverse->clear();
        }

    private:
        ForwardMapPtr forward;
        InverseMapPtr inverse;
        InverseBiMapPtr inverseAccess;
    };

    template<typename T, typename U>
    class BidirectionalMap {
    private:
        using Map = BiMapImpl<T, U>;
        using InverseMap = BiMapImpl<U, T>;
        using MapPtr = std::shared_ptr<Map>;

        void swap(BidirectionalMap &a, BidirectionalMap &b) {
            // swap only data pointers, inverse access pointers remain unchanged
            std::swap(a.biMap->forward, b.biMap->forward);
            std::swap(a.biMap->inverse, b.biMap->inverse);
        }

    public:

        BidirectionalMap() : biMap(new Map) {
            constructPair();
        }

        BidirectionalMap(const BidirectionalMap &other) : biMap(new Map(Map::Construct::Empty)) {
            copyResources(other);
            constructPair();
        }

        BidirectionalMap(BidirectionalMap &&other) noexcept : BidirectionalMap() {
            swap(*this, other);
        }


        BidirectionalMap &operator=(BidirectionalMap other) {
            swap(*this, other);
            return *this;
        }

        ~BidirectionalMap() {
            // break reference cycle
            if (nullptr != biMap) {
                biMap->inverseAccess = nullptr;
            }
        }

        // converters

        BidirectionalMap(const Map &map) : biMap(new Map(Map::Construct::Empty)) {
            copyResources(map);
            constructPair();
        }

        operator Map &() {
            return *biMap;
        };

        operator const Map &() const {
            return *biMap;
        }

        // access

        Map *operator->() noexcept {
            return biMap.get();
        }

        const Map *operator->() const noexcept {
            return biMap.get();
        }

        Map &operator*() noexcept {
            return *biMap;
        }

        const Map &operator*() const noexcept {
            return *biMap;
        }

        typename Map::Iterator begin() const noexcept(noexcept(biMap->begin())) {
            return biMap->begin();
        }

        typename Map::Iterator end() const noexcept(noexcept(biMap->end())) {
            return biMap->end();
        }

    private:
        void constructPair() {
            auto inverse = std::shared_ptr<InverseMap>(new InverseMap(biMap->inverse, biMap->forward, biMap));
            biMap->inverseAccess = inverse;
        }


        void copyResources(const Map &other) {
            biMap->forward = std::make_shared<typename Map::ForwardMap>(*other.forward);
            biMap->inverse = std::make_shared<typename Map::InverseMap>(*other.inverse);
        }

        void copyResources(const BidirectionalMap &other) {
            copyResources(*other.biMap);
        }

        MapPtr biMap;
    };
}

#endif //BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
