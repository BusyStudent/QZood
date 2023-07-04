#pragma once

#include <optional>
#include <memory>
#include <QList>

template <typename T>
using Optional = std::optional<T>;
template <typename T>
using Result = std::optional<T>;
template <typename T>
using RefPtr = std::shared_ptr<T>;
template <typename T>
using WeakPtr = std::weak_ptr<T>;
template <typename T>
using ResultPtr = Result<RefPtr<T> >;

class DynRefable : public std::enable_shared_from_this<DynRefable> {
    public:
        virtual ~DynRefable() = default;

        template <typename T>
        RefPtr<T> as() {
            return std::dynamic_pointer_cast<T>(shared_from_this());
        }
        template <typename T>
        bool      inherts() const {
            auto ptr = shared_from_this();
            return dynamic_cast<const T*>(ptr) != nullptr;
        }
    protected:
        DynRefable() = default;
};