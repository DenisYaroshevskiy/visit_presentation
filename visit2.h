#include <algorithm>
#include <array>
#include <type_traits>

namespace v2 {
namespace tools {

// standard algorithms but constexpr -----------------------

template <typename I, typename V>
constexpr I find(I f, I l, const V& v) {
  while (f != l) {
    if (*f == v)
      break;
    ++f;
  }
  return f;
}

// type as value staff
// ----------------------------------------------------------

template <typename T>
struct type_ {
  using type = T;
};

struct error_base {};

template <typename T>
constexpr bool is_error_v = std::is_base_of_v<error_base, T>;

template <typename T>
constexpr auto is_error_f(const T) -> std::bool_constant<is_error_v<T>> {
  return {};
}

template <typename T>
constexpr auto unwrap(T x) {
  if constexpr (is_error_v<T>) {
    return x;
  } else {
    return typename T::type{};
  }
}

// Typelist --------------------------------------------------------

template <size_t idx, typename T>
struct _indexed_type {};

template <typename...>
struct _type_list_impl;

template <size_t... from0_to_n, typename... Ts>
struct _type_list_impl<std::index_sequence<from0_to_n...>, Ts...>
    : _indexed_type<from0_to_n, Ts>... {};

template <typename... Ts>
struct type_list : _type_list_impl<std::index_sequence_for<Ts...>, Ts...> {
  static constexpr std::size_t size() { return sizeof...(Ts); }
};

template <size_t idx, typename T>
constexpr type_<T> _get_success(const _indexed_type<idx, T>&) {
  return {};
}

template <size_t idx, typename Where>
struct index_is_out_of_bounds : error_base {};

template <size_t idx, typename... Ts>
constexpr auto get(type_list<Ts...>) {
  constexpr type_list<Ts...> t;
  if constexpr (idx < t.size()) {
    return _get_success<idx>(t);
  } else {
    return index_is_out_of_bounds<idx, decltype(t)>{};
  }
}

template <typename... Ts>
constexpr size_t _find_index_of_first_error(type_list<Ts...>) {
  constexpr type_list<Ts...> t;
  if constexpr (!t.size()) {
    return 0;
  } else {
    constexpr std::array checks{is_error_v<Ts>...};

    return static_cast<size_t>(find(checks.begin(), checks.end(), true) -
                               checks.begin());
  }
}

struct no_errors_found {};

template <typename... Ts>
constexpr auto get_first_error(type_list<Ts...>) {
  constexpr type_list<Ts...> t;
  constexpr size_t error_idx = _find_index_of_first_error(t);
  if constexpr (error_idx < t.size()) {
    return unwrap(get<error_idx>(t));
  } else {
    return no_errors_found{};
  }
}

template <typename T, typename = void>
struct _common_type_exists : std::false_type {};

template <typename... Ts>
struct _common_type_exists<type_list<Ts...>,
                           std::void_t<std::common_type_t<Ts...>>>
    : std::true_type {};

template <typename... Ts>
struct no_common_type : error_base {};

template <typename... Ts>
constexpr auto common_type(type_list<Ts...>) {
  constexpr type_list<Ts...> t;
  if constexpr (auto error = get_first_error(t); is_error_f(error)) {
    return error;
  } else if constexpr (!_common_type_exists<type_list<Ts...>>{}) {
    return no_common_type<Ts...>{};
  } else {
    return type_<std::common_type_t<Ts...>>{};
  }
}

template <size_t i>
struct index_ : std::integral_constant<size_t, i> {};

template <typename Op, size_t... from0_to_n>
constexpr auto _make_type_list(Op op, std::index_sequence<from0_to_n...>) {
  constexpr type_list<decltype(unwrap(op(index_<from0_to_n>{})))...> t{};
  if constexpr (auto error = get_first_error(t); is_error_f(error)) {
    return error;
  } else {
    return t;
  }
}

template <size_t size, typename Op>
constexpr auto make_type_list(Op op) {
  return _make_type_list(op, std::make_index_sequence<size>{});
}

template <size_t size, typename Op, size_t... from0_to_n>
constexpr auto _make_array(Op op, std::index_sequence<from0_to_n...>) {
  if constexpr (auto types = make_type_list<size>(op); is_error_f(types)) {
    return types;
  } else if constexpr (auto result_tv = common_type(types);
                       is_error_f(result_tv)) {
    return result_tv;
  } else {
    using R = typename decltype(result_tv)::type;
    return std::array<R, size> {
      static_cast<R>(op(index_<from0_to_n>{})) ...
    };
  }
}

template <size_t size, typename Op>
constexpr auto make_array(Op op) {
  return _make_array<size>(op, std::make_index_sequence<size>{});
}

/*

auto _visit(Op&& op, Vs&& ...vs) {
  using return_type = _visit_return_type<decltype(std::forward<Op>(op)),
decltype(std::forward<Vs>(vs)) ...>;

}
*/

/*

struct null_{
    friend constexpr std::bool_constant<true> operator==(null_, null_) { return
{}; } friend constexpr std::bool_constant<false> operator!=(null_, null_) {
return {}; }

    template <typename T>
    friend constexpr std::bool_constant<false> operator==(const T&, null_) {
return {}; } template <typename T> friend constexpr std::bool_constant<false>
operator==(null_, const T&) { return {}; }

    template <typename T>
    friend constexpr std::bool_constant<true> operator!=(const T&, null_) {
return {}; } template <typename T> friend constexpr std::bool_constant<true>
operator!=(null_, const T&) { return {}; }
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
struct _type_list_impl<std::index_sequence<from0_to_n...>, Ts...> :
_type_list_element<i, Ts> ... {};

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
auto _cfmap_array(std::array<T, size> x, Op op,
std::index_sequence<from0_to_n...>) { constexpr auto res_value_type =
common_type(type_<decltype(op(x[from0_to_n]))>{}...); if constexpr
(res_value_type == null_{}) { return null_{}; } else { return
std::array<res_value_type, size> { op(x[from0_to_n])...
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
