#include <array>
#include <numeric>
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
  signature vtable[] {
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

// There is no such thing in mathematics but for our case
// this works
template <typename I, typename O, typename T>
constexpr O inner_product_inverse(I f, I l, O o, T product) {
  while (f != l) {
    *o++ = product / *f;
    product %= *f;
    ++f;
  }
  return o;
}

template <size_t... dimensions>
constexpr auto compute_multipliers_array() {
  std::array res{dimensions...};
  running_sum_shifted(res.rbegin(), res.rend(), res.rbegin(), 1,
                      std::multiplies<>{});
  return res;
}

template <size_t size>
inline constexpr auto to_multi_dimensional_array_helper(
    size_t idx,
    const std::array<size_t, size>& multipliers) {
  auto res = multipliers;
  inner_product_inverse(multipliers.begin(), multipliers.end(),
                        res.begin(), idx);
  return res;
}

template <typename T, size_t... dimensions>
struct table {
  static constexpr size_t total_size = (dimensions * ...);

  static constexpr std::array multipliers_array =
      compute_multipliers_array<dimensions...>();

  template <size_t... from0_to_n>
  static constexpr auto multipliers_sequence_helper(
      std::index_sequence<from0_to_n...>) {
    return std::index_sequence<multipliers_array[from0_to_n]...>{};
  }

  using multipliers_sequence = decltype(multipliers_sequence_helper(
      std::make_index_sequence<sizeof...(dimensions)>{}));

  static constexpr size_t to_one_dimensional(
      std::array<size_t, sizeof...(dimensions)> idxes) {
    return inner_product(idxes.begin(), idxes.end(), multipliers_array.begin(),
                         0);
  }

  template <size_t... idxes>
  static constexpr size_t to_one_dimensional(std::index_sequence<idxes...>) {
    return to_one_dimensional(std::array{idxes...});
  }

  template <size_t idx>
  static constexpr auto to_multy_dimensional_array =
      to_multi_dimensional_array_helper(idx, multipliers_array);

  template <size_t idx, size_t... from0_to_n>
  static constexpr auto to_multi_dimensional_sequence_helper(
      std::index_sequence<from0_to_n...>) {
    return std::index_sequence<
        to_multy_dimensional_array<idx>[from0_to_n]...>{};
  }

  template <size_t idx>
  using to_multi_dimensional_sequence =
      decltype(to_multi_dimensional_sequence_helper<idx>(
          std::make_index_sequence<sizeof...(dimensions)>{}));

  std::array<T, total_size> data;

  constexpr T& operator[](std::array<size_t, sizeof...(dimensions)> idxs) {
    return data[to_one_dimensional(idxs)];
  }

  constexpr const T& operator[](
      std::array<size_t, sizeof...(dimensions)> idxs) const {
    return data[to_one_dimensional(idxs)];
  }

  template <size_t... idxs>
  constexpr T& operator[](std::index_sequence<idxs...> _idxs) {
    return data[to_one_dimensional(_idxs)];
  }

  template <size_t... idxs>
  constexpr const T& operator[](std::index_sequence<idxs...> _idxs) const {
    return data[to_one_dimensional(_idxs)];
  }
};

template <size_t idx, typename Table, typename OutTable, typename Op>
constexpr void table_map_one_helper(const Table& t, OutTable& out_table, Op op) {
  using multi_index = typename Table::template to_multi_dimensional_sequence<idx>;
  out_table.data[idx] = op(multi_index{}, t.data[idx]);
}

template <typename Table, typename OutTable, typename Op, size_t... from0_to_n>
constexpr void table_map_helper(const Table& t,
                                OutTable& out_table,
                                Op op,
                                std::index_sequence<from0_to_n...>) {
  (table_map_one_helper<from0_to_n>(t, out_table, op), ...);
}

template <typename T, typename Op, size_t... dimensions>
constexpr auto table_map(table<T, dimensions...> t, Op op) {
  using U = decltype(op(std::index_sequence<dimensions - 1 ...>{}, t.data[0]));
  table<U, dimensions...> res{};
  table_map_helper(t, res, op, std::make_index_sequence<t.data.size()>{});
  return res;
}

}  // namespace tools