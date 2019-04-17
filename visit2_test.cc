#include "visit2.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

namespace v2 {
namespace tools {

TEST_CASE("visit2.is_error") {
    static_assert(is_error_v<error<int>>);
    static_assert(!is_error_v<int>);
}

}  // namespace tools
}  // namespace v2