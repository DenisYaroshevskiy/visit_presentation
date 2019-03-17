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

template <typename T>
constexpr void is_same_test(T, T) {}

TEST_CASE("visit, simplified") {
  constexpr std::variant<int, char> v = 0;
  bool is_int = false;
  bool is_char = false;

  simplified::visit(
      overload{[&](int) { is_int = true; }, [&](char) { is_char = true; }}, v);
  REQUIRE(is_int);
  REQUIRE(!is_char);
}

TEST_CASE("visit, table_index_math") {
  constexpr table_index_math<3, 4, 2> t{};
  static_assert(t.size_linear == 3 * 4 * 2);
  REQUIRE(t.multipliers_a == std::array<size_t, 3>{4 * 2, 2, 1});

  REQUIRE(t.as_linear({2, 3, 1}) == 3 * 4 * 2 - 1);
  static_assert(t.as_linear(std::index_sequence<2, 3, 1>{}) == 23);

  REQUIRE(t.as_multi_a(2 * 4 * 2 + 3 * 2 + 1) ==
          std::array<size_t, 3>{2u, 3u, 1u});

  for (size_t first = 0; first < 3; ++first) {
    for (size_t second = 0; second < 4; ++second) {
      for (size_t third = 0; third < 2; ++third) {
        size_t as_linear = t.as_linear({first, second, third});
        decltype(t)::index_a res;
        t.notation.to(as_linear, res.begin());
        REQUIRE(std::array<size_t, 3>{first, second, third} == res);
      }
    }
  }

  is_same_test(t.as_multi_s<0>(), std::index_sequence<0, 0, 0>{});
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
  constexpr explicit P(std::integer_sequence<T, idx1, idx2>)
      : first{idx1}, second{idx2} {}

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

TEST_CASE("visit, table") {
  using test_t = P<size_t>;

  constexpr auto init = make_table<2, 3>([](auto seq) { return test_t{seq}; });

  static_assert(init[std::index_sequence<0, 2>{}] == test_t{0, 2});
  static_assert(init[std::index_sequence<1, 1>{}] == test_t{1, 1});

  for (size_t first = 0; first < 2; ++first) {
    for (size_t second = 0; second < 3; ++second) {
      REQUIRE(init.data[first * 3 + second] == test_t{first, second});
    }
  }

  constexpr table<bool, 2, 3> check =
      table_map(init, [](auto seq, test_t x) { return test_t{seq} == x; });

  REQUIRE(std::all_of(check.data.begin(), check.data.end(),
                      [](bool x) { return x; }));
}

TEST_CASE("visit, type_table_helpers") {
  is_same_test(any_typelist_helper(std::index_sequence<0, 1, 2>{}),
               type_list<size_t, size_t, size_t>{});
  is_same_test(any_type_table_helper<2, 2>(),
               type_table<type_list<size_t, size_t, size_t, size_t>, 2, 2>{});
  auto mapped = table_map(any_type_table_helper<2, 2>(), [](auto, auto) -> size_t { return 0u; });
  is_same_test(decltype(mapped)::data{}, type_list<size_t, size_t, size_t, size_t>{});
}

TEST_CASE("visit, type_table") {
  constexpr auto ttable = make_type_table<2, 3>([](auto seq) {
    return std::index_sequence<get<0>(seq), get<1>(seq)>{};
  });

  is_same_test(get<0>(ttable), type_t<std::index_sequence<0, 0>>{});
  is_same_test(get<3>(ttable), type_t<std::index_sequence<1, 0>>{});
}

TEST_CASE("visit, visit with R") {
  {
    using test_t = std::variant<int, char>;

    constexpr auto visitor =
        overload{[](int, int) { return 0; }, [](int, char) { return 1; },
                 [](char, int) { return 2; }, [](char, char) { return 3; }};

    static_assert(visit_with_r<int>(visitor, test_t{3}, test_t{'a'}) == 1);
  }
  {
    struct Throws {
      Throws() { throw std::runtime_error("aaaa"); }
    };

    std::variant<Throws, int> v{3};
    try {
      v.emplace<Throws>();
    } catch (...) {}

    try {
      visit_with_r<void>([](const auto&) { }, v);
      REQUIRE(false);
    } catch (const std::bad_variant_access&) {}
  }
}

}  // namespace tools
