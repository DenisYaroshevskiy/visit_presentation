#include "visit.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace doctest {
template <typename T, size_t size>
struct StringMaker<std::array<T, size>> {
  static String convert(const std::array<T, size>& arr) {
    return String("{") +
           std::accumulate(arr.begin(), arr.end(), doctest::String{},
                           [](String sum, const T& elem) {
                             return sum + toString(elem) + ", ";
                           }) +
           String("}");
  }
};
}  // namespace doctest

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

  REQUIRE(t.multipliers_array == std::array<size_t, 3>{4 * 2, 2, 1});
  REQUIRE(t.to_one_dimensional({2, 3, 1}) == 3 * 4 * 2 - 1);
  static_assert(t.to_one_dimensional(std::index_sequence<2, 3, 1>{}) == 23);

  REQUIRE(t.to_multy_dimensional_array<(2 * 4 * 2) + (3 * 2) + 1> == std::array<size_t, 3>{2u, 3u, 1u});

  for (size_t first = 0; first < 3; ++first) {
      for (size_t second = 0; second < 4; ++second) {
        for (size_t third = 0; third < 2; ++third) {
          size_t as_single = t.to_one_dimensional({first, second, third});
          auto back = to_multi_dimensional_array_helper(as_single, t.multipliers_array);
          REQUIRE(std::array<size_t, 3>{first, second, third} == back);
        }
      }
  }
}

template <size_t idx, size_t... sequence>
constexpr size_t get(std::index_sequence<sequence...>) {
  std::array arr{sequence...};
  return arr[idx];
}

// std::pair is not sufficiently constexpr
template <typename T>
struct P {
  T first = 0;
  T second = 0;

  constexpr P() {}

  constexpr P(T first, T second) : first(first), second(second) {}

  template <T idx1, T idx2>
  constexpr explicit P(std::integer_sequence<T, idx1, idx2>) : first{idx1}, second{idx2} {}

  friend constexpr bool operator==(const P& x, const P& y) {
    return x.first == y.first && x.second == y.second;
  }

  friend std::ostream& operator<<(std::ostream& out, const P& x) {
    out << '{' << x.first << ", " << x.second << '}';
    return out;
  }

  friend doctest::String toString(const P& p) {
    using namespace doctest;
    return String("{") + toString(p.first) + ", " + toString(p.second) + "}";
  }
};

TEST_CASE("visit, table, public") {
  using test_t = P<size_t>;
  constexpr table<test_t, 2, 3> t{};
  constexpr auto init = table_map(t, [](auto seq, test_t)  {
    return test_t{seq};
  });

  static_assert(init[std::index_sequence<0, 2>{}] == test_t{0, 2});
  static_assert(init[std::index_sequence<1, 1>{}] == test_t{1, 1});

  for (size_t first = 0; first < 2; ++first) {
    for (size_t second = 0; second < 3; ++second) {
      REQUIRE(init.data[first * 3 + second] == test_t{first, second});
    }
  }

  constexpr table<bool, 2, 3> check = table_map(init, [](auto seq, test_t x) {
     return test_t{seq} == x;
  });

  REQUIRE(std::all_of(check.data.begin(), check.data.end(), [](bool x) { return x; }));
}

}  // namespace tools