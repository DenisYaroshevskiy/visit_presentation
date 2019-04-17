#include "visit2.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace v2 {
namespace tools {
namespace {

template <typename T>
constexpr void is_same_test(T, T) {}


TEST_CASE("visit2.is_error") {
  struct some_error : error_base {};

  static_assert(is_error_v<some_error>);
  static_assert(!is_error_v<int>);
}

TEST_CASE("visit2.type_list") {
    using List = type_list<int, char, double>;
    constexpr List t;
    is_same_test(get<1>(t), type_<char>{});
    static_assert(is_error_v<decltype(get<3>(t))>);
    is_same_test(get<3>(t), index_is_out_of_bounds<3, List>{});
}

}  // namespace
}  // namespace tools
}  // namespace v2