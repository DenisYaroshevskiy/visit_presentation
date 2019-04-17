#include <array>
#include <type_traits>

namespace v2 {
namespace tools {

struct _error_base {};

template <typename ...>
struct error : _error_base {};


template <typename T>
constexpr bool is_error_v = std::is_base_of_v<_error_base, T>;

/*

struct null_{
    friend constexpr std::bool_constant<true> operator==(null_, null_) { return {}; }
    friend constexpr std::bool_constant<false> operator!=(null_, null_) { return {}; }

    template <typename T>
    friend constexpr std::bool_constant<false> operator==(const T&, null_) { return {}; }
    template <typename T>
    friend constexpr std::bool_constant<false> operator==(null_, const T&) { return {}; }

    template <typename T>
    friend constexpr std::bool_constant<true> operator!=(const T&, null_) { return {}; }
    template <typename T>
    friend constexpr std::bool_constant<true> operator!=(null_, const T&) { return {}; }
};

template <size_t i>
struct index_ : std::integral_constant<size_t, i> {};

template <typename T>
struct type_ {
    using type = T;
};

template <size_t i, typename T>
struct _type_list_element {
    constexpr type_<T> operator[](index_<i>) { return {}; }
};

template <typename ...>
struct _type_list_impl;

template <size_t...from0_to_n, typename ...Ts>
struct _type_list_impl<std::index_sequence<from0_to_n...>, Ts...> : _type_list_element<i, Ts> ... {};

template <size_t i, typename T>
type_<T> _get_element(const _type_list_element<i, T>&) { return {}; }

template <typename ...Ts>
struct type_list : _type_list_impl<std::index_sequence_for<Ts...>, Ts...> {
    constexpr size_t size() const {
        return sizeof ...(Ts);
    }

    template <size_t i>
    constexpr auto operator[](index_<i>) const {
        if (i >= size()) { return null_{}; }
        return _get_element<i>(*this);
    }
};

template <typename ...Ts>
constexpr bool has_nulls(type_list<Ts...>) {
    std::array all_checks = {
        std::is_same_v<Ts, null_> ...
    };

    // std::any_of is not constexpr;
    for (bool check : all_checks) {
        if (check) return false;
    }
    return true;
}

template <typename, typename = void>
struct _common_type_helper {
    using type = null_;
};

template <typename... Ts>
struct _common_type_helper<type_list<Ts...>,
                        std::void_t<std::common_type_t<Ts...>>> {
  using type = type_<std::common_type_t<Ts...>>;
};

template <typename ...Ts>
constexpr auto common_type(type_list<Ts...>) {
    constexpr type_list<Ts...> t;
    if constexpr (has_nulls(t)) {
        return null_;
    }
    return typename _common_type_helper<Ts...>::type{};
}

template <typename T, size_t size, typename Op, size_t ... from0_to_n>
auto _cfmap_array(std::array<T, size> x, Op op, std::index_sequence<from0_to_n...>) {
    constexpr auto res_value_type = common_type(type_<decltype(op(x[from0_to_n]))>{}...);
    if constexpr (res_value_type == null_{}) {
        return null_{};
    } else {
        return std::array<res_value_type, size> {
            op(x[from0_to_n])...
        };
    }
}

template <typename T, size_t size, typename Op>
auto cfmap(std::array<T, size> x, Op op) {
    return _cfmap_array(x, op, std::make_index_sequence<size>{});
}

*/

}  // namespace  tools
}  // namespace v2
