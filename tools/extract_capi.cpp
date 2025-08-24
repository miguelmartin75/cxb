#include <cxb/cxb.h>
#include <stdio.h>

#include <fmt/format.h>

struct Foo {
    double x;
};

void format_value(Arena* a, String8& dst, String8 args, Foo x) {
    dst.extend(a, S8_LIT("Foo(x="));
    format_value(a, dst, {}, x.x);
    dst.extend(a, S8_LIT(")"));
}

int fib(int n) {
    if(n <= 1) return n;
    int n1 = fib(n-1);
    int n2 = fib(n - 2);
    println("fib({}) = fib({}) + fib({}) = {} + {} = {}", n, n - 1, n - 2, n1, n2, n1 + n2);
    return n1 + n2;
}

int main(int argc, char* argv[]) {
    Foo f = {};
    f.x = -1.42;
    // println("Hello world: {.12f}, {}, {}", 2.5, 2, f);
    println("Hello world: {.2}", f.x);
    println("Hello world: {}", false);

    // fib(30);
}
