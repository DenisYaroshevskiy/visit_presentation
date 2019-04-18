#include "visit2.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace v2 {
namespace tools {
namespace {

template <typename T>
constexpr void is_same_test(T, T) {}

struct some_error : error_base {};

TEST_CASE("visit2.is_error") {
  static_assert(is_error_v<some_error>);
  static_assert(!is_error_v<int>);
}

TEST_CASE("visit2.type_list.get") {
  using List = type_list<int, char, double>;
  constexpr List t;
  is_same_test(get<1>(t), type_<char>{});
  static_assert(is_error_v<decltype(get<3>(t))>);
  is_same_test(get<3>(t), index_is_out_of_bounds<3, List>{});
}

TEST_CASE("visit2.type_list.get_first_error") {
  {
    using List = type_list<int, some_error, double>;
    is_same_test(get_first_error(List{}), some_error{});
  }
  {
    struct other_error : error_base {};
    using List = type_list<int, some_error, double, other_error, float>;
    is_same_test(get_first_error(List{}), some_error{});
  }
  {
    using List = type_list<>;
    is_same_test(get_first_error(List{}), no_errors_found{});
  }
}

TEST_CASE("visit2.common_type") {
  {
    using List = type_list<int, char>;
    is_same_test(common_type(List{}), type_<int>{});
  }
  {
    using List = type_list<>;
    is_same_test(common_type(List{}), no_common_type<>{});
  }
  {
    struct A{};
    using List = type_list<int, A>;
    is_same_test(common_type(List{}), no_common_type<int, A>{});
  }
}

}  // namespace
}  // namespace tools
}  // namespace v2