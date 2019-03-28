#include <array>
#include <numeric>
#include <type_traits>
#include <variant>

namespace tools {

template <typename... Fs>
struct overload : Fs... {
  using Fs::operator()...;
};

template <class... Fs>
overload(Fs...)->overload<Fs...>;

namespace simplified {

template <typename F, typename... Ts>
constexpr void visit(F f, const std::variant<Ts...>& v) {
  using signature = void (*)(F, const std::variant<Ts...>&);

  // clang-format off
  constexpr signature vtable[] {
      [](F f, const std::variant<Ts...>& v) { f(std::get<Ts>(v)); } ...
  };
  // clang-format on

  vtable[v.index()](f, v);
}

}  // namespace simplified

// Very similar to std::partial_sum
template <typename I, typename O, typename V, typename Op>
constexpr O running_sum_shifted(I f, I l, O o, V sum, Op op) {
  while (f != l) {
    auto tmp = op(sum, *f++);
    *o++ = std::move(sum);
    sum = std::move(tmp);
  }
  return o;
}

// std::inner_product but constexpr
template <class InputIt1, class InputIt2, class T>
constexpr T inner_product(InputIt1 first1,
                          InputIt1 last1,
                          InputIt2 first2,
                          T init) {
  while (first1 != last1) {
    init = std::move(init) + *first1 * *first2;  // std::move since C++20
    ++first1;
    ++first2;
  }
  return init;
}

template <typename I, typename P>
constexpr bool all_of(I f, I l, P p) {
  while (f != l) {
    if (!p(*f++)) {
      return false;
    }
  }
  return true;
}

template <typename I>
using ValueType = typename std::iterator_traits<I>::value_type;

template <typename I>
// requires InputIterator<I>
class varying_notation {
  I powers_f_;
  I powers_l_;

 public:
  constexpr varying_notation(I powers_f, I powers_l)
      : powers_f_{powers_f}, powers_l_{powers_l} {}

  template <typename T, typename O>
  // requires Number<T> && OutputIterator<O>
  constexpr O to(T number, O o) const {
    for (I power_i = powers_f_; power_i != powers_l_; ++power_i) {
      *o++ = number / *power_i;
      number %= *power_i;
    }
    return o;
  }

  template <typename I2>
  // requires InputIterator<I2>
  constexpr ValueType<I2> from(I2 f, I2 l) const {
    return inner_product(f, l, powers_f_, ValueType<I2>{0});
  }
};

template <typename T>
struct type_t {
  using type = T;
};

struct null_t {};

template <typename T, size_t>
struct indexed_t {};

template <typename...>
struct type_list_impl;

template <size_t... idxs, typename... Ts>
struct type_list_impl<std::index_sequence<idxs...>, Ts...>
    : indexed_t<Ts, idxs>... {};

template <typename... Ts>
struct type_list : type_list_impl<std::index_sequence_for<Ts...>, Ts...> {};

template <typename, typename = void>
struct common_type_impl {
  using type = null_t;
};

template <typename... Ts>
struct common_type_impl<type_list<Ts...>,
                        std::void_t<std::common_type_t<Ts...>>> {
  using type = type_t<std::common_type_t<Ts...>>;
};

template <typename... Ts>
auto common_type(type_list<Ts...> t) {
  return typename common_type_impl<type_list<Ts...>>::type{};
}

template <size_t I, typename T>
constexpr type_t<T> get(const indexed_t<T, I>&) {
  return {};
}

template <size_t... dims>
constexpr auto compute_multipliers_a() {
  std::array res{dims...};
  running_sum_shifted(res.rbegin(), res.rend(), res.rbegin(), 1,
                      std::multiplies<>{});
  return res;
}

template <size_t... dims>
struct table_index_math {
  using index_a = std::array<size_t, sizeof...(dims)>;

  template <size_t... idxs>
  using index_s = std::index_sequence<idxs...>;

  static constexpr size_t size_linear = (dims * ...);
  static constexpr std::array multipliers_a = compute_multipliers_a<dims...>();
  static constexpr varying_notation notation{std::begin(multipliers_a),
                                             std::end(multipliers_a)};

  static constexpr size_t as_linear(const index_a& arr) {
    return notation.from(arr.begin(), arr.end());
  }

  template <size_t... idxs>
  static constexpr size_t as_linear(const index_s<idxs...>&) {
    return as_linear(index_a{idxs...});
  }

  static constexpr auto as_multi_a(size_t idx) {
    index_a res{};
    notation.to(idx, res.begin());
    return res;
  }

  template <size_t idx, size_t... from0_to_n>
  static constexpr auto as_multi_s_helper(std::index_sequence<from0_to_n...>) {
    constexpr index_a as_a = as_multi_a(idx);
    return index_s<as_a[from0_to_n]...>{};
  }

  template <size_t idx>
  static constexpr auto as_multi_s() {
    return as_multi_s_helper<idx>(std::make_index_sequence<sizeof...(dims)>{});
  };
};

template <typename T, size_t... dims>
struct table {
  using math = table_index_math<dims...>;
  static constexpr size_t size_linear = math::size_linear;

  using index_a = typename math::index_a;

  template <size_t... idxs>
  using index_s = typename math::template index_s<idxs...>;

  std::array<T, size_linear> data;

  static constexpr size_t as_linear(const index_a& idxs) {
    return math::as_linear(idxs);
  }

  template <size_t... _idxs>
  static constexpr size_t as_linear(const index_s<_idxs...>& idxs) {
    return math::as_linear(idxs);
  }

  constexpr T& operator[](const index_a& idxs) {
    return data[math::as_linear(idxs)];
  }

  constexpr const T& operator[](const index_a& idxs) const {
    return data[math::as_linear(idxs)];
  }

  template <size_t... _idxs>
  constexpr T& operator[](index_s<_idxs...> idxs) {
    return data[math::as_linear(idxs)];
  }

  template <size_t... _idxs>
  constexpr const T& operator[](index_s<_idxs...> idxs) const {
    return data[math::as_linear(idxs)];
  }
};

template <typename OutTable, typename Table, typename Op, size_t... from0_to_n>
constexpr OutTable table_map_helper(const Table& t,
                                    Op op,
                                    std::index_sequence<from0_to_n...>) {
  using math = typename Table::math;
  return OutTable{[&] {
    constexpr auto multi_s = math::template as_multi_s<from0_to_n>();
    return op(multi_s, t.data[from0_to_n]);
  }()...};
}

template <typename T, size_t... dims, typename Op>
constexpr auto table_map(table<T, dims...> t, Op op) {
  using U = decltype(op(std::index_sequence<dims - 1 ...>{}, t.data[0]));
  return table_map_helper<table<U, dims...>>(
      t, op, std::make_index_sequence<t.data.size()>{});
}

template <size_t... dims, typename Op>
constexpr auto make_table(Op op) {
  return table_map(table<int, dims...>{},
                   [&](auto seq, int) { return op(seq); });
}

template <typename TypeList, size_t... dims>
struct type_table : TypeList, table_index_math<dims...> {
  using data = TypeList;

  template <typename UTypeList>
  using same_dims_table = type_table<UTypeList, dims...>;
};

template <typename Table, typename Op, size_t... from0_to_n>
constexpr auto type_table_map_helper(Table,
                                     Op op,
                                     std::index_sequence<from0_to_n...>) {
  using UTypeList =
      type_list<typename decltype(op(Table::template as_multi_s<from0_to_n>(),
                                     get<from0_to_n>(Table{})))::type...>;
  return typename Table::template same_dims_table<UTypeList>{};
}

template <typename TypeList, size_t... dims, typename Op>
constexpr auto table_map(type_table<TypeList, dims...> t, Op op) {
  return type_table_map_helper(t, op,
                               std::make_index_sequence<t.size_linear>{});
}

template <size_t... from0_to_n>
constexpr auto any_typelist_helper(std::index_sequence<from0_to_n...>) {
  return type_list<decltype(from0_to_n)...>{};
}

template <size_t... dims, typename TypeList>
constexpr auto any_type_table_type_list_and_dims_helper(TypeList) {
  return type_table<TypeList, dims...>{};
}

template <size_t... dims>
constexpr auto any_type_table_helper() {
  constexpr table_index_math<dims...> math{};
  return any_type_table_type_list_and_dims_helper<dims...>(
      any_typelist_helper(std::make_index_sequence<math.size_linear>{}));
}

template <size_t... dims, typename Op>
constexpr auto make_type_table(Op op) {
  return table_map(any_type_table_helper<dims...>(),
                   [&](auto seq, auto) { return op(seq); });
}

template <typename R, typename FwdOp, typename... FwdVs>
struct visit_vtable_generator {
  using vtable_element = R (*)(FwdOp, FwdVs...);

  template <size_t... idxs>
  constexpr vtable_element operator()(std::index_sequence<idxs...>) const {
    return [](FwdOp op, FwdVs... vs) -> R {
      return std::forward<FwdOp>(op)(
          std::get<idxs>(std::forward<FwdVs>(vs))...);
    };
  }
};

template <typename R, typename Op, typename... Vs>
constexpr R visit_with_r_simplified(Op&& op, Vs&&... vs) {
  constexpr visit_vtable_generator<R, decltype(std::forward<Op>(op)),
                                   decltype(std::forward<Vs>(vs))...>
      vtable_generator;

  constexpr auto vtable =
      make_table<std::variant_size_v<std::decay_t<Vs>>...>(vtable_generator);

  return vtable[{vs.index()...}](std::forward<Op>(op), std::forward<Vs>(vs)...);
}

template <typename R, typename Op, typename... Vs>
constexpr R visit_with_r(Op&& op, Vs&&... vs) {
  constexpr visit_vtable_generator<R, decltype(std::forward<Op>(op)),
                                   decltype(std::forward<Vs>(vs))...>
      vtable_generator;

  constexpr auto vtable =
      make_table<std::variant_size_v<std::decay_t<Vs>>...>(vtable_generator);

  size_t idx = vtable.as_linear({vs.index()...});
  if (idx >= vtable.size_linear) {
    throw std::bad_variant_access{};
  }

  return vtable.data[idx](std::forward<Op>(op), std::forward<Vs>(vs)...);
}

template <typename T>
struct is_variant : std::false_type {};

template <typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};

template <typename T>
constexpr bool is_variant_v = is_variant<T>::value;

template <typename R, typename FwdOp, typename... FwdVs>
struct should_enable_visit_r_helper {
  template <size_t... idxs>
  constexpr bool operator()(std::index_sequence<idxs...>) {
    return std::is_invocable_r_v<
        R, FwdOp, decltype(std::get<idxs>(std::declval<FwdVs>()))...>;
  }
};

template <typename R, typename FwdOp, typename... FwdVs>
constexpr bool should_enable_visit_r() {
  if constexpr (!sizeof...(FwdVs) ||
                (!is_variant_v<std::decay_t<FwdVs>> || ...)) {
    return false;
  } else {
    constexpr should_enable_visit_r_helper<R, FwdOp, FwdVs...> helper;

    constexpr auto for_each_scenario =
        make_table<std::variant_size_v<std::decay_t<FwdVs>>...>(helper);

    return tools::all_of(for_each_scenario.data.begin(),
                         for_each_scenario.data.end(),
                         [](bool x) { return x; });
  }
}

template <typename FwdOp, typename... FwdVs>
struct visit_return_type_mapper {
  template <size_t... idxs>
  constexpr auto operator()(std::index_sequence<idxs...>) {
    if constexpr ((!is_variant_v<std::decay_t<FwdVs>> || ...)) {
      return type_t<null_t>{};
    } else if constexpr (!std::is_invocable_v<FwdOp,
                                              decltype(std::get<idxs>(
                                                  std::declval<FwdVs>()))...>) {
      return type_t<null_t>{};
    } else {
      return type_t<decltype(
          std::declval<FwdOp>()(std::get<idxs>(std::declval<FwdVs>())...))>{};
    }
  }
};

template <typename FwdOp, typename... FwdVs>
constexpr auto visit_return_types_helper() {
  constexpr visit_return_type_mapper<FwdOp, FwdVs...> mapper;
  constexpr auto result_table =
      make_type_table<std::variant_size_v<std::decay_t<FwdVs>>...>(mapper);

  return tools::common_type(typename decltype(result_table)::data{});
}

template <typename FwdOp, typename... FwdVs>
using visit_return_type =
    typename decltype((visit_return_types_helper<FwdOp, FwdVs...>()))::type;

template <typename R, typename Op, typename... Vs>
constexpr auto visit(Op&& op, Vs&&... vs) -> std::enable_if_t<
    should_enable_visit_r<R,
                          decltype(std::forward<Op>(op)),
                          decltype(std::forward<Vs>(vs))...>(),
    R> {
  return visit_with_r<R>(std::forward<Op>(op), std::forward<Vs>(vs)...);
}

template <typename Op, typename... Vs>
constexpr auto visit(Op&& op, Vs&&... vs)
    -> visit_return_type<decltype(std::forward<Op>(op)),
                         decltype(std::forward<Vs>(vs))...> {
  using R = visit_return_type<decltype(std::forward<Op>(op)),
                              decltype(std::forward<Vs>(vs))...>;
  return visit_with_r<R>(std::forward<Op>(op), std::forward<Vs>(vs)...);
}

}  // namespace tools