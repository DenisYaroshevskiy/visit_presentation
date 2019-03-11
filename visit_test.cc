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
}

}  // namespace tools