#ifndef TAGGED_H
#define TAGGED_H

#include <compare>
#include <cstddef>
#include <functional>
#include <utility>

namespace util {

/**
 *  using Address = util::Tagged<std::string, AddressTag>;
 *  using Name = util::Tagged<std::string, NameTag>;
 *  Address и Name - разные типы (как explicit)
 *
 *  TODO: read about overhead
 */
template <typename Value, typename Tag>
class Tagged {
public:
    using ValueType = Value;

    explicit Tagged(Value&& v)
        : value_(std::move(v)) {
    }
    explicit Tagged(const Value& v)
        : value_(v) {
    }

    const Value& operator*() const {
        return value_;
    }

    Value& operator*() {
        return value_;
    }

    // C++20, будет вызван соответствующий оператор для поля value_
    auto operator<=>(const Tagged<Value, Tag>&) const = default;

private:
    Value value_;
};

// Хешер. Чтобы Tagged-объекты можно было хранить в unordered-контейнерах
template <typename TaggedValue>
struct TaggedHasher {
    size_t operator()(const TaggedValue& value) const {
        return std::hash<typename TaggedValue::ValueType>{}(*value);
    }
};

}  // namespace util

#endif  // TAGGED_H