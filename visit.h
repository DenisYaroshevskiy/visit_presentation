#include <array>
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
/*
template <typename T, size_t size>
struct array {
  T data[size];

  using iterator = T*;
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using reference = T&;
  using const_reference = const T&;

  constexpr iterator begin() { return data; }
  constexpr const_iterator begin() const { return data; }
  constexpr const_iterator cbegin() const { return begin(); }

  constexpr iterator end() { return data + size; }
  constexpr const_iterator end() const { return data + size; }
  constexpr const_iterator cend() const { return end(); }

  constexpr reverse_iterator rbegin() { return reverse_iterator{end()}; }
  constexpr const_reverse_iterator rbegin() const {
    return reverse_iterator{end()};
  }
  constexpr const_reverse_iterator crbegin() const {
    return reverse_iterator{end()};
  }

  constexpr reverse_iterator rend() { return reverse_iterator{begin()}; }
  constexpr const_reverse_iterator rend() const {
    return reverse_iterator{begin()};
  }
  constexpr const_reverse_iterator crend() const {
    return reverse_iterator{begin()};
  }

  reference operator[](size_t idx) { return data[idx]; }
  const_reference operator[](size_t idx) const { return data[idx]; }
};

template <class T, class... U>
array(T, U...)->array<T, 1 + sizeof...(U)>;
*/

// Very similar to std::partial_sum
template <typename I, typename O, typename V, typename Op>
constexpr O running_sum(I f, I l, O o, V sum, Op op) {
  while (f != l) {
    auto tmp = op(sum, *f++);
    *o++ = std::move(sum);
    sum = std::move(tmp);
  }
  return o;
}

template <size_t... dimensions>
constexpr auto compute_multipliers_as_array() {
  std::array res{dimensions...};
  running_sum(res.rbegin(), res.rend(), res.rbegin(), 1, std::multiplies<>{});
  return res;
}

template <typename T, size_t... dimensions>
struct table {
  static constexpr size_t total_size = (dimensions * ...);

  static constexpr std::array multipliers_array =
      compute_multipliers_as_array<dimensions...>();

  template <size_t... idxs>
  static constexpr auto multipliers_sequence_helper(std::index_sequence<idxs...>) {
    return std::index_sequence<multipliers_array[idxs]...>{};
  }

  using multipliers_sequence = decltype(multipliers_sequence_helper(
      std::make_index_sequence<sizeof...(dimensions)>{}));

  std::array<T, total_size> data;
};

}  // namespace tools