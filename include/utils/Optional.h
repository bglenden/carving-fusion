/**
 * Simple optional implementation for C++14 compatibility
 * (std::optional is C++17)
 */

#pragma once

#include <stdexcept>
#include <utility>

namespace ChipCarving {

template <typename T>
class Optional {
 private:
  alignas(T) unsigned char storage_[sizeof(T)];
  bool has_value_;

 public:
  Optional() : has_value_(false) {}

  explicit Optional(const T& value) : has_value_(true) {
    new (storage_) T(value);
  }

  explicit Optional(T&& value) : has_value_(true) {
    new (storage_) T(std::move(value));
  }

  Optional(const Optional& other) : has_value_(other.has_value_) {
    if (has_value_) {
      new (storage_) T(*other);
    }
  }

  Optional(Optional&& other) noexcept : has_value_(other.has_value_) {
    if (has_value_) {
      new (storage_) T(std::move(*other));
      other.reset();
    }
  }

  ~Optional() {
    reset();
  }

  Optional& operator=(const Optional& other) {
    if (this != &other) {
      reset();
      has_value_ = other.has_value_;
      if (has_value_) {
        new (storage_) T(*other);
      }
    }
    return *this;
  }

  Optional& operator=(Optional&& other) noexcept {
    if (this != &other) {
      reset();
      has_value_ = other.has_value_;
      if (has_value_) {
        new (storage_) T(std::move(*other));
        other.reset();
      }
    }
    return *this;
  }

  // Assignment from T
  Optional& operator=(const T& value) {
    reset();
    has_value_ = true;
    new (storage_) T(value);
    return *this;
  }

  Optional& operator=(T&& value) {
    reset();
    has_value_ = true;
    new (storage_) T(std::move(value));
    return *this;
  }

  bool has_value() const {
    return has_value_;
  }
  explicit operator bool() const {
    return has_value_;
  }

  T& value() {
    if (!has_value_) {
      throw std::runtime_error("Optional has no value");
    }
    return *reinterpret_cast<T*>(storage_);
  }

  const T& value() const {
    if (!has_value_) {
      throw std::runtime_error("Optional has no value");
    }
    return *reinterpret_cast<const T*>(storage_);
  }

  T& operator*() {
    return value();
  }

  const T& operator*() const {
    return value();
  }

  T* operator->() {
    return &value();
  }

  const T* operator->() const {
    return &value();
  }

  T value_or(const T& default_value) const {
    return has_value_ ? value() : default_value;
  }

  void reset() {
    if (has_value_) {
      reinterpret_cast<T*>(storage_)->~T();
      has_value_ = false;
    }
  }
};

// Helper function similar to std::make_optional
template <typename T>
Optional<T> make_optional(T&& value) {
  return Optional<T>(std::forward<T>(value));
}

}  // namespace ChipCarving
