#pragma once

#include <type_traits>
#include <concepts>

namespace kat::utils {
    template<typename T>
    concept scoped_enum = std::is_scoped_enum_v<T>;

    template<scoped_enum T>
    struct Flags {
        using bits_t = T;
        using value_t = std::underlying_type_t<T>;

        value_t value;

        inline explicit constexpr Flags(const value_t& value) : value(value) {};
        inline explicit constexpr Flags(value_t&& value) : value(std::move(value)) {};
        inline explicit constexpr Flags(const T& bit) : value(static_cast<value_t>(bit)) {};

        inline explicit constexpr operator value_t() const noexcept { // NOLINT(*-explicit-constructor)
            return value;
        };

        inline constexpr value_t get() const noexcept {
            return value;
        };

        inline constexpr bool check(T bit) const noexcept {
            return (value & static_cast<value_t>(bit)) == static_cast<value_t>(bit);
        };

        inline constexpr bool check(const Flags<T>& bit) const noexcept {
            return (value & bit.value) == bit.value;
        };

        inline constexpr Flags<T> operator|(const Flags<T>& rhs) {
            return Flags<T>(value | rhs.value);
        };

        inline constexpr Flags<T> operator|(const T& rhs) {
            return Flags<T>(value | static_cast<value_t>(rhs));
        };

        inline constexpr Flags<T> operator&(const Flags<T>& rhs) const noexcept {
            return Flags<T>(value & rhs.value);
        };

        inline constexpr Flags<T> operator&(const T& rhs) const noexcept {
            return Flags<T>(value & static_cast<value_t>(rhs));
        };
    };

#define KAT_GENFLAGS(flagtype, bittype) using flagtype = ::kat::utils::Flags<bittype>; inline constexpr ::kat::utils::Flags<bittype> operator|(const bittype& a, const bittype& b) noexcept { return flagtype(static_cast<flagtype::value_t>(a) | static_cast<flagtype::value_t>(b)); };  inline constexpr ::kat::utils::Flags<bittype> operator|(const bittype& a, const flagtype& b) noexcept { return flagtype(static_cast<flagtype::value_t>(a) | b.value); }; inline constexpr ::kat::utils::Flags<bittype> operator&(const bittype& a, const bittype& b) noexcept { return flagtype(static_cast<flagtype::value_t>(a) & static_cast<flagtype::value_t>(b)); };  inline constexpr ::kat::utils::Flags<bittype> operator&(const bittype& a, const flagtype& b) noexcept { return flagtype(static_cast<flagtype::value_t>(a) & b.value); }
}