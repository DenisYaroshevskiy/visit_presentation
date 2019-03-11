#include "visit.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace tools {

TEST_CASE("visit, simplified") {
  constexpr std::variant<int, char> v = 0;
  bool is_int = false;
  bool is_char = false;

  simplified::visit(
      overload{[&](int) { is_int = true; }, [&](char) { is_char = true; }}, v);
  REQUIRE(is_int);
  REQUIRE(!is_char);
}


TEST_CASE("visit, table, internal types") {
  constexpr table<int, 3, 4, 2> t{};
  static_assert(t.total_size == 3 * 4 * 2);
  REQUIRE(t.multipliers_array == std::array<size_t, 3>{4 * 2, 2, 1});
  static_assert(std::is_same_v<decltype(t)::multipliers_sequence,
                               std::index_sequence<4 * 2, 2, 1>>);

  REQUIRE(t.to_one_dimensional({2, 3, 1}) == 3 * 4 * 2 - 1);
  static_assert(t.to_one_dimensional(std::index_sequence<2, 3, 1>{}) == 23);

  REQUIRE(t.to_multy_dimensional_array<2 * 3 * 1>[0] == 2);
}

template <size_t idx, size_t ...sequence>
constexpr size_t get(std::index_sequence<sequence...>) {
  std::array arr{sequence...};
  return arr[idx];
}

// std::pair is not sufficiently constexpr
template <typename T>
struct P {
  T first;
  T second;

  friend constexpr bool operator==(const P& x, const P& y) {
    return x.first == y.first && x.second == y.second;
  }

  friend std::ostream& operator<<(std::ostream& out, const P& x) {
    out << '{' << x.first << ", " << x.second << '}';
    return out;
  }
};

TEST_CASE("visit, table, public") {
  using test_t = P<size_t>;
  constexpr table<test_t, 2, 3> t{};
  constexpr auto init = table_map(t, [](auto seq, test_t) -> test_t {
    return {get<0>(seq), get<1>(seq)};
  });

  REQUIRE(init[std::index_sequence<0, 2>{}] == test_t{0, 2});
}

}  // namespace tools