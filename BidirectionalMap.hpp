//
// Created by tim on 16.06.21.
//

#ifndef BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
#define BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
#include <unordered_map>
#include <memory>

namespace BiMap {
    template<typename T, typename U>
    class BidirectionalMap {
    public:
        using ForwardKey = T;
        using InverseKey = U;
    private:
        using ForwardMap = std::unordered_map<T, const U*>;
        using InverseMap = std::unordered_map<U, const T*>;
        using ForwardMapPtr = std::shared_ptr<ForwardMap>;
        using InverseMapPtr = std::shared_ptr<InverseMap>;
        explicit BidirectionalMap(BidirectionalMap<InverseKey , ForwardKey > &inverseMap) :
                forward(inverseMap.inverse), inverse(inverseMap.forward) {}

        enum class Dummy {
            D
        };

        explicit constexpr BidirectionalMap(Dummy) : forward(nullptr), inverse(nullptr) {}

        static void swap(BidirectionalMap &m1, BidirectionalMap &m2) {
            std::swap(m1.forward, m2.forward);
            std::swap(m1.inverse, m2.inverse);
        }

    public:
        class Iterator {
        public:
            using IteratorType = decltype(std::declval<ForwardMap>().cbegin());
            using ValueType = std::pair<const ForwardKey&, const InverseKey&>;

            Iterator() = default;

            explicit Iterator(const BidirectionalMap &map) noexcept(noexcept(map.forward->begin())) :
                    it(map.forward->begin()) {}

            explicit Iterator(IteratorType it) noexcept : it(it) {}

            Iterator &operator++() noexcept(noexcept(++std::declval<IteratorType>())) {
                ++it;
                return *this;
            }

            bool operator==(const Iterator &other) const noexcept(noexcept(it == it)) {
                return this->it == other.it;
            }

            bool operator!=(const Iterator &other) const noexcept(noexcept(*this == other)) {
                return !(*this == other);
            }

            ValueType operator*() const noexcept(noexcept(ValueType(it->first, *it->second))) {
                return {it->first, *it->second};
            }

            auto operator->() const noexcept -> IteratorType {
                return it;
            }

        private:
            friend class BidirectionalMap;
            IteratorType it;
        };

        BidirectionalMap() : forward(std::make_shared<ForwardMap>()), inverse(std::make_shared<InverseMap>()) {}

        BidirectionalMap(const BidirectionalMap &other) : forward(std::make_shared<ForwardMap>(*other.forward)),
            inverse(std::make_shared<InverseMap>(*other.inverse)) {}

        BidirectionalMap(BidirectionalMap &&other) noexcept : BidirectionalMap(Dummy::D) {
            swap(*this, other);
        }

        BidirectionalMap &operator=(BidirectionalMap other) {
            swap(*this, other);
            return *this;
        }

        ~BidirectionalMap() = default;

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

        auto invert() noexcept(noexcept(BidirectionalMap<InverseKey, ForwardKey>(*this)))
            -> BidirectionalMap<InverseKey, ForwardKey> {
            return BidirectionalMap<InverseKey, ForwardKey>(*this);
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

            inverse->erase(inverse->find(*pos->second));
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

    private:
        ForwardMapPtr forward;
        InverseMapPtr inverse;
        friend class BidirectionalMap<InverseKey, ForwardKey>;
    };
}

#endif //BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
