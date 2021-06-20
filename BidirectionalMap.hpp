//
// Created by tim on 16.06.21.
//

#ifndef BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
#define BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
#include <unordered_map>
#include <memory>

namespace BiMap::implementation {
    template<typename T>
    class AllocOncePointer {
        AllocOncePointer() : data(nullptr), owner(false) {}
    public:
        explicit AllocOncePointer(T *data) : data(data), owner(false) {}

        template<typename ...ARGS>
        explicit AllocOncePointer(ARGS &&...args) : data(new T(std::forward<ARGS>(args)...)), owner(true) {}

        AllocOncePointer(const AllocOncePointer &other) : data(other.data), owner(false) {}

        void swap(AllocOncePointer &other) noexcept {
            std::swap(data, other.data);
            std::swap(owner, other.owner);
        }

        AllocOncePointer(AllocOncePointer &&other) noexcept : AllocOncePointer() {
            swap(other);
        }

        AllocOncePointer &operator=(AllocOncePointer other) {
            swap(other);
            return *this;
        }

        ~AllocOncePointer() {
            if (owner) {
                delete data;
            }
        }

        [[nodiscard]] bool isOwner() const noexcept {
            return owner;
        }

        T &operator*() {
            return *data;
        }

        const T &operator*() const {
            return *data;
        }

        T *operator->() {
            return data;
        }

        const T *operator->() const {
            return data;
        }
    private:
        T *data;
        bool owner{};
    };

    template<typename T>
    void swap(AllocOncePointer<T> &a, AllocOncePointer<T> &b) noexcept {
        a.swap(b);
    }
}

namespace BiMap {
    template<typename ForwardKey, typename InverseKey>
    class BidirectionalMap {
    private:
        using ForwardMap = std::unordered_map<ForwardKey, const InverseKey*>;
        using InverseMap = std::unordered_map<InverseKey, const ForwardKey*>;
        using ForwardMapPtr = std::shared_ptr<ForwardMap>;
        using InverseMapPtr = std::shared_ptr<InverseMap>;
        using InverseBiMap = BidirectionalMap<InverseKey, ForwardKey>;
        friend InverseBiMap;
        using InversBiMapPtr = implementation::AllocOncePointer<InverseBiMap>;
        friend class implementation::AllocOncePointer<BidirectionalMap>;
        explicit BidirectionalMap(InverseBiMap &inverseMap) :
                forward(inverseMap.inverse), inverse(inverseMap.forward), inverseAccess(&inverseMap) {}

        enum class Construct {
            Empty
        };

        explicit constexpr BidirectionalMap(Construct) : forward(nullptr), inverse(nullptr),
                                                         inverseAccess(static_cast<InverseBiMap*>(nullptr)) {}

    public:
        class Iterator {
        public:
            using IteratorType = decltype(std::declval<ForwardMap>().cbegin());
            using ValueType = std::pair<const ForwardKey&, const InverseKey&>;

            Iterator() = default;

            explicit Iterator(IteratorType it) : it(it),
                val(this->it == IteratorType() ? nullptr : std::make_shared<ValueType>(it->first, *it->second)) {}

            explicit Iterator(const BidirectionalMap &map) : Iterator(map.forward->begin()) {}

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
            friend class BidirectionalMap;
            IteratorType it;
            std::shared_ptr<ValueType> val;
        };

        BidirectionalMap() : forward(std::make_shared<ForwardMap>()), inverse(std::make_shared<InverseMap>()),
            inverseAccess(*this) {}

        BidirectionalMap(const BidirectionalMap &other) : forward(std::make_shared<ForwardMap>(*other.forward)),
            inverse(std::make_shared<InverseMap>(*other.inverse)), inverseAccess(*this) {}

        void swap(BidirectionalMap &other) {
            std::swap(this->forward, other.forward);
            std::swap(this->inverse, other.inverse);
            this->inverseAccess.swap(other.inverseAccess);
        }


        BidirectionalMap(BidirectionalMap &&other) noexcept : BidirectionalMap(Construct::Empty) {
            swap(other);
        }

        BidirectionalMap &operator=(BidirectionalMap other) {
            swap(other);
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

        auto invert() -> InverseBiMap & {
            return *inverseAccess;
        }

        auto invert() const -> const InverseBiMap & {
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

    private:
        ForwardMapPtr forward;
        InverseMapPtr inverse;
        InversBiMapPtr inverseAccess;
    };
}

#endif //BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
