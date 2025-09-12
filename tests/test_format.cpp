#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cxb/cxb.h>

TEST_CASE("int", "[format_value]") {
    {
        AArenaTmp tmp = begin_scratch();
        String8 dst = {};
        format_value(tmp.arena, dst, {}, 123);
        REQUIRE(dst == S8_LIT("123"));
    }

    {
        AArenaTmp tmp = begin_scratch();
        String8 dst = {};
        format_value(tmp.arena, dst, {}, -123);
        REQUIRE(dst == S8_LIT("-123"));
    }
}

TEST_CASE("float", "[format_value]") {
    {
        AArenaTmp tmp = begin_scratch();
        String8 dst = {};
        format_value(tmp.arena, dst, {}, 0.1);
        REQUIRE(dst == S8_LIT("0.1"));
    }

    {
        AArenaTmp tmp = begin_scratch();
        String8 dst = {};
        format_value(tmp.arena, dst, {}, -0.1);
        REQUIRE(dst == S8_LIT("-0.1"));
    }
}

TEST_CASE("float args constraint", "[format_value]") {
    {
        AArenaTmp tmp = begin_scratch();
        String8 dst = {};
        format_value(tmp.arena, dst, S8_LIT(".2f"), 0.12345);
        REQUIRE(dst == S8_LIT("0.12"));
    }

    {
        AArenaTmp tmp = begin_scratch();
        String8 dst = {};
        format_value(tmp.arena, dst, S8_LIT(".2f"), -0.12345);
        REQUIRE(dst == S8_LIT("-0.12"));
    }
}
