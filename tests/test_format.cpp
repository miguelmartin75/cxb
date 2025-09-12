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

TEST_CASE("format_value formats various types", "[format]") {
    ArenaTmp scratch = begin_scratch();
    String8 dst = arena_push_string8(scratch.arena, 1);
    format_value(scratch.arena, dst, S8_LIT(""), "hi");
    format_value(scratch.arena, dst, S8_LIT(""), S8_LIT(" there"));
    format_value(scratch.arena, dst, S8_LIT(""), true);
    format_value(scratch.arena, dst, S8_LIT(""), 1.5f);
    format_value(scratch.arena, dst, S8_LIT(""), 2.25);
    REQUIRE(dst == S8_LIT("hi theretrue1.52.25"));
    end_scratch(scratch);
}
