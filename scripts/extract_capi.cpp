#include <cxb/cxb.h>
#include <fmt/format.h>
#include <stdio.h>

struct Foo {
    double x;
};

void format_value(Arena* a, String8& dst, String8 args, Foo x) {
    (void) args;
    dst.extend(a, S8_LIT("Foo(x="));
    format_value(a, dst, {}, x.x);
    dst.extend(a, S8_LIT(")"));
}

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    Foo f = {};
    f.x = -1.42;

    println("Hello world, f= {}", f);
    println("f.x={.2}", f.x);
    println("bool: {}", false);
    return 0;
}
