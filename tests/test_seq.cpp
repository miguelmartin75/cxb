#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#define CXB_IMPL
#include "cxb.h"


TEST_CASE( "foo", "[Foo]" ) {
    REQUIRE(foo() == 42);
}
