#include "visit3.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace v3 {
namespace {

template <typename T> constexpr void is_same_test(T, T) {}

TEST_CASE("visit3.make_type_list") {
  is_same_test(make_types<>(), types<>{});
  is_same_test(make_types<int, char>(), types<int, char>{});
  is_same_test(make_types<int, char, error>(), error{});
}

TEST_CASE("From the bottom") { visit(0, 1, 2); }

template <size_t... idxs> constexpr auto to_array(idxs_<idxs...>) {
  return std::array{idxs...};
}

template <size_t idx, typename Conversions>
int convert_back_and_forth_test(Conversions) {
  constexpr Conversions conv;
  constexpr auto as_muli = conv.template multi<idx>();
  static_assert(conv.linear(to_array(as_muli)) == idx);
  return 0;
}

template <size_t... idxs, typename Conversions>
void convert_back_and_forth_tests(idxs_<idxs...>, Conversions conv) {
  (convert_back_and_forth_test<idxs>(conv), ...);
}

TEST_CASE("visit, index_conversions") {
  constexpr index_conversions<3, 4, 2> conv{};

  REQUIRE(conv.pows == std::array<size_t, 3>{4 * 2, 2, 1});

  static_assert(conv.linear({2, 3, 1}) == 3 * 4 * 2 - 1);

  is_same_test(conv.multi<2 * 4 * 2 + 3 * 2 + 1>(), idxs_<2u, 3u, 1u>{});

  is_same_test(conv.multi<0>(), std::index_sequence<0, 0, 0>{});

  convert_back_and_forth_tests(make_idxs_<23>{}, conv);
}

} // namespace
} // namespace v3