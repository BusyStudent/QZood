#pragma once

#include <optional>
#include <memory>

template <typename T>
using Optional = std::optional<T>;
template <typename T>
using Result = std::optional<T>;
template <typename T>
using RefPtr = std::shared_ptr<T>;
template <typename T>
using ResultPtr = Result<RefPtr<T> >;
