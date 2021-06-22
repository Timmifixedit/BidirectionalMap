//
// Created by tim on 16.06.21.
//

#ifndef BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP
#define BIDIRECTIONALMAP_BIDIRECTIONALMAP_HPP

#include <unordered_map>
#include <memory>
#include <optional>

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
            template<typename T, typename U> typename ForwardMapType = std::unordered_map,
            template<typename T, typename U> typename InverseMapType = std::unordered_map>
    class BidirectionalMap {
    private:
        using ForwardMap = ForwardMapType<ForwardKey, const InverseKey *>;
        using InverseMap = InverseMapType<InverseKey, const ForwardKey *>;
        using ForwardMapPtr = std::shared_ptr<ForwardMap>;
        using InverseMapPtr = std::shared_ptr<InverseMap>;
        using InverseBiMap = BidirectionalMap<InverseKey, ForwardKey, InverseMapType, ForwardMapType>;
        friend InverseBiMap;
        using InversBiMapPtr = implementation::AllocOncePointer<InverseBiMap>;
        friend class implementation::AllocOncePointer<BidirectionalMap>;

        static_assert(std::is_default_constructible<ForwardMap>::value &&
            std::is_default_constructible<InverseMap>::value, "Map base containers must be default constructible.");
        static_assert(std::is_copy_constructible<ForwardMap>::value &&
            std::is_copy_constructible<InverseMap>::value, "Map base containers must be copy constructible.");

        explicit BidirectionalMap(InverseBiMap &inverseMap) noexcept :
                forward(inverseMap.inverse), inverse(inverseMap.forward), inverseAccess(&inverseMap) {}

        enum class Construct {
            Empty
        };

        explicit constexpr BidirectionalMap(Construct) noexcept: forward(nullptr), inverse(nullptr),
                                                                 inverseAccess(*this) {}

    public:
        class Iterator {
        public:
            using IteratorType = decltype(std::declval<ForwardMap>().cbegin());
            using ValueType = std::pair<const ForwardKey &, const InverseKey &>;

            Iterator() noexcept(noexcept(IteratorType())) : it(), val(std::nullopt), container(nullptr), end(true) {}

            Iterator(IteratorType it, const ForwardMap &container) : it(it), container(&container),
                end(this->it == std::end(container)) {
                if (!end) {
                    val.emplace(it->first, *it->second);
                }
            }

            explicit Iterator(const BidirectionalMap &map) : Iterator(map.forward->begin(), *map.forward) {}

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
                                            noexcept(this->val.emplace(this->it->first, *this->it->second))) {
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
            // swap data pointers
            std::swap(this->forward, other.forward);
            std::swap(this->inverse, other.inverse);
            // swap data pointers in inverse lookup
            std::swap(this->inverseAccess->forward, other.inverseAccess->forward);
            std::swap(this->inverseAccess->inverse, other.inverseAccess->inverse);
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

        [[nodiscard]] auto size() const noexcept(noexcept(forward->size())) {
            return forward->size();
        }

        [[nodiscard]] bool empty() const noexcept(noexcept(forward->empty())) {
            return forward->empty();
        }

        void reserve(std::size_t n) noexcept(noexcept(forward->reserve(n)) && noexcept(inverse->reserve(n))) {
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

        constexpr Iterator end() const noexcept(noexcept(Iterator())) {
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

        bool operator!=(const BidirectionalMap &other) const noexcept(noexcept(*this == other)) {
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
