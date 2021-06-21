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
        constexpr AllocOncePointer() noexcept: data(nullptr), owner(false) {}

    public:
        explicit AllocOncePointer(T *data) : data(data), owner(false) {}

        template<typename ...ARGS>
        explicit AllocOncePointer(ARGS &&...args) : data(new T(std::forward<ARGS>(args)...)), owner(true) {}

        AllocOncePointer(const AllocOncePointer &other) noexcept: data(other.data), owner(false) {}

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

        ~AllocOncePointer() {
            if (owner) {
                delete data;
            }
        }

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
    template<typename ForwardKey, typename InverseKey,
        template<typename T, typename U>typename Map = std::unordered_map>
    class BidirectionalMap {
    private:
        using ForwardMap = Map<ForwardKey, const InverseKey *>;
        using InverseMap = Map<InverseKey, const ForwardKey *>;
        using ForwardMapPtr = std::shared_ptr<ForwardMap>;
        using InverseMapPtr = std::shared_ptr<InverseMap>;
        using InverseBiMap = BidirectionalMap<InverseKey, ForwardKey, Map>;
        friend InverseBiMap;
        using InversBiMapPtr = implementation::AllocOncePointer<InverseBiMap>;

        friend class implementation::AllocOncePointer<BidirectionalMap>;

        explicit BidirectionalMap(InverseBiMap &inverseMap) noexcept :
                forward(inverseMap.inverse), inverse(inverseMap.forward), inverseAccess(&inverseMap) {}

        enum class Construct {
            Empty
        };

        explicit constexpr BidirectionalMap(Construct) noexcept : forward(nullptr), inverse(nullptr),
                                                         inverseAccess(static_cast<InverseBiMap *>(nullptr)) {}

    public:
        class Iterator {
        public:
            using IteratorType = decltype(std::declval<ForwardMap>().cbegin());
            using ValueType = std::pair<const ForwardKey &, const InverseKey &>;

            Iterator() noexcept(noexcept(IteratorType())) : it(), val(nullptr), container(nullptr), end(true) {}

            explicit Iterator(IteratorType it, const ForwardMap &container) :
                    it(it),
                    val(this->it == container.end() ? nullptr : std::make_shared<ValueType>(it->first, *it->second)),
                    container(&container), end(this->it == container.end()) {}

            explicit Iterator(const BidirectionalMap &map) : Iterator(map.forward->begin(), *map.forward) {}

            Iterator &operator++() {
                if (end) {
                    return *this;
                }

                ++it;
                if (it != container->end()) {
                    val = std::make_shared<ValueType>(it->first, *it->second);
                } else {
                    val = nullptr;
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

            auto operator->() const noexcept {
                return val.get();
            }

        private:
            friend class BidirectionalMap;
            IteratorType it;
            std::shared_ptr<ValueType> val;
            const ForwardMap *container;
            bool end;
        };

        BidirectionalMap() : forward(std::make_shared<ForwardMap>()), inverse(std::make_shared<InverseMap>()),
                             inverseAccess(*this) {}

        template<typename InputIt>
        BidirectionalMap(InputIt start, InputIt end) : BidirectionalMap() {
            while (start != end) {
                emplace(*start);
                ++start;
            }
        }

        BidirectionalMap(std::initializer_list<std::pair<ForwardKey, InverseKey>> init) :
            BidirectionalMap(init.begin(), init.end()) {}

        BidirectionalMap(const BidirectionalMap &other) : forward(std::make_shared<ForwardMap>(*other.forward)),
                                                          inverse(std::make_shared<InverseMap>(*other.inverse)),
                                                          inverseAccess(*this) {}

        void swap(BidirectionalMap &other) noexcept {
            std::swap(this->forward, other.forward);
            std::swap(this->inverse, other.inverse);
            this->inverseAccess.swap(other.inverseAccess);
        }


        BidirectionalMap(BidirectionalMap &&other) noexcept : BidirectionalMap(Construct::Empty) {
            swap(other);
        }

        BidirectionalMap &operator=(BidirectionalMap other) noexcept {
            swap(other);
            return *this;
        }

        ~BidirectionalMap() = default;

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

            auto[it, _] = forward->emplace(std::move(tmp.first), nullptr);
            decltype(inverse->begin()) invIt;
            std::tie(invIt, _) = inverse->emplace(std::move(tmp.second), &it->first);
            it->second = &invIt->first;
            return {Iterator(it, *forward), true};
        }

        [[nodiscard]] auto size() const noexcept {
            return forward->size();
        }

        [[nodiscard]] bool empty() const {
            return forward->empty();
        }

        void reserve(std::size_t n) {
            forward->reserve(n);
            inverse->reserve(n);
        }

        auto invert() noexcept -> InverseBiMap & {
            return *inverseAccess;
        }

        auto invert() const noexcept -> const InverseBiMap & {
            return *inverseAccess;
        }

        Iterator begin() const noexcept(noexcept(Iterator(*this))) {
            return Iterator(*this);
        }

        Iterator end() const noexcept(noexcept(Iterator())) {
            return Iterator();
        }

        Iterator find(const ForwardKey &key) const {
            return Iterator(forward->find(key), *forward);
        }

        Iterator erase(Iterator pos) {
            if (pos == end()) {
                return pos;
            }

            inverse->erase(inverse->find(pos->second));
            return Iterator(forward->erase(pos.it), *forward);
        }

        std::size_t erase(const ForwardKey &key) {
            auto it = Iterator(forward->find(key), *forward);
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

        bool operator!=(const BidirectionalMap &other) const {
            return !(*this == other);
        }

        void clear() noexcept(noexcept(forward->clear()) && noexcept(inverse->clear())) {
            forward->clear();
            inverse->clear();
        }

    private:
        ForwardMapPtr forward;
        InverseMapPtr inverse;
        InversBiMapPtr inverseAccess;
    };
}

#endif //BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
