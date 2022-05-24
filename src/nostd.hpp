// Data Output Unit Firmware / Darius Kellermann <kellermann@protonmail.com>

#ifndef NOSTD_HPP_
#define NOSTD_HPP_

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

using Size = std::ptrdiff_t;

using i8 = int_fast8_t;

template <typename Tp_>
constexpr auto to_integral(Tp_ enumerator) ->
    typename std::underlying_type<Tp_>::type {
  return static_cast<typename std::underlying_type<Tp_>::type>(enumerator);
}

enum class u8 : uint8_t {};

inline constexpr u8 operator~(const u8 lhs) {
  return static_cast<u8>(~to_integral(lhs));
}

inline constexpr u8 operator&(const u8 lhs, const u8 rhs) {
  return static_cast<u8>(to_integral(lhs) & to_integral(rhs));
}

inline constexpr u8 operator|(const u8 lhs, const u8 rhs) {
  return static_cast<u8>(to_integral(lhs) | to_integral(rhs));
}

inline constexpr u8 operator^(const u8 lhs, const u8 rhs) {
  return static_cast<u8>(to_integral(lhs) ^ to_integral(rhs));
}

inline constexpr u8 operator<<(const u8 lhs, const unsigned rhs) {
  return static_cast<u8>(to_integral(lhs) << rhs);
}

inline constexpr u8 operator>>(const u8 lhs, const unsigned rhs) {
  return static_cast<u8>(to_integral(lhs) >> rhs);
}

enum class u16 : uint16_t {};

inline constexpr u16 operator~(const u16 lhs) {
  return static_cast<u16>(~to_integral(lhs));
}

inline constexpr u16 operator&(const u16 lhs, const u16 rhs) {
  return static_cast<u16>(to_integral(lhs) & to_integral(rhs));
}

inline constexpr u8 operator&(const u16 lhs, const u8 rhs) {
  return static_cast<u8>(to_integral(lhs) & static_cast<uint8_t>(rhs));
}

inline constexpr u16 operator|(const u16 lhs, const u16 rhs) {
  return static_cast<u16>(to_integral(lhs) | to_integral(rhs));
}

inline constexpr u16 operator|(const u16 lhs, const u8 rhs) {
  return static_cast<u16>(to_integral(lhs) | to_integral(rhs));
}

inline constexpr u16 operator>>(const u16 lhs, const unsigned rhs) {
  return static_cast<u16>(to_integral(lhs) >> rhs);
}

inline constexpr u16 operator<<(const u16 lhs, const unsigned rhs) {
  return static_cast<u16>(to_integral(lhs) << rhs);
}

enum class u32 : uint32_t {};

inline constexpr u32 operator>>(const u32 lhs, const unsigned rhs) {
  return static_cast<u32>(to_integral(lhs) >> rhs);
}

template <typename Tp_> struct Register { intptr_t address; };

template <typename Tp_> inline Tp_ load(const Register<Tp_> a_register) {
  return *reinterpret_cast<volatile Tp_ *>(a_register.address);
}

template <typename Tp_>
inline void store(const Register<Tp_> a_register, Tp_ const val) {
  *reinterpret_cast<volatile Tp_ *>(a_register.address) = val;
}

template <typename Tp_, Size nm_> class Array {
  public:
    using value_type = Tp_;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using size_type = decltype(nm_);
    using difference_type = std::ptrdiff_t;

    static_assert(nm_ > 0);

    Tp_ items_[static_cast<std::make_unsigned<Size>::type>(nm_)];

    void fill(const Tp_ &value_to_fill) {
      std::fill(begin(), size(), value_to_fill);
    }

    constexpr iterator begin() noexcept { return data(); }
    constexpr const_iterator begin() const noexcept { return data(); }
    constexpr iterator end() noexcept { return data() + nm_; }
    constexpr const_iterator end() const noexcept { return data() + nm_; }

    constexpr size_type size() const noexcept { return nm_; }
    constexpr size_type max_size() const noexcept { return nm_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return false; }

    constexpr reference operator[](const size_type index) noexcept {
      return items_[index];
    }

    constexpr const_reference operator[](const size_type index) const noexcept {
      return items_[index];
    }

    constexpr reference front() noexcept { return *begin(); }
    constexpr const_reference front() const noexcept { return *begin(); }
    constexpr reference back() noexcept { return *(end() - 1); }
    constexpr const_reference back() const noexcept { return *(end() - 1); }

    constexpr pointer data() { return items_; }
    constexpr const_pointer data() const { return items_; }
};

#endif // NOSTD_HPP_
