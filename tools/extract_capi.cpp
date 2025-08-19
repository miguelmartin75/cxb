#include <cxb/cxb.h>
#include <stdio.h>

template <typename T, typename... Args>
void _format_impl(Arena* a, String8& dst, const char* fmt, const T& first, const Args&... rest) {
    String8 s = {};
    s.data = (char*) fmt;

    i64 args_i = -1;
    u64 i = 0;
    while(*fmt) {
        s.len += 1;

        char curr = s[i];
        if(curr == '{') {
            args_i = i;
        } else if(curr == '}') {
            String8 args = s.slice(args_i + 1, (i64)i - 1);
            format_value(a, dst, args, first);
            _format_impl(a, dst, s.data + i + 1, rest...);
            break;
        } else if(args_i < 0) {
            string8_push_back(dst, a, curr);
        }
        ++i;
        ++fmt;
    }
}

// TODO: fixme
void _format_impl(Arena* a, String8& dst, const char* fmt) {
    while(*fmt) string8_push_back(dst, a, *fmt++);
}

template <typename... Args>
String8 format(Arena* a, const char* fmt, const Args&... args) {
    String8 dst = arena_push_string8(a);
    _format_impl(a, dst, fmt, args...);
    return dst;
}

template <typename... Args>
String8 format(const char* fmt, const Args&... args) {
    Arena* a = arena_make_nbytes(KB(1)); // TODO: scratch buffer
    return format(a, fmt, args...);
}

template <typename... Args>
void print(FILE* f, Arena* a, const char* fmt, const Args&... args) {
    String8 str = format(a, fmt, args...);
    if(str.data) {
        fwrite(&str[0], sizeof(char), str.len, f);
    }
}

template <typename... Args>
void print(FILE* f, const char* fmt, const Args&... args) {
    String8 str = format(fmt, args...);
    if(str.data) {
        fwrite(&str[0], sizeof(char), str.len, f);
    }
}

template <typename... Args>
inline void print(const char* fmt, const Args&... args) {
    print(stdout, fmt, args...);
}

template <typename... Args>
inline void println(const char* fmt, const Args&... args) {
    auto str = format(fmt, args...);
    print("{}\n", str);
}

// template <class T> void format_value(Arena* a, String8& dst, String8 args, T x);

void format_value(Arena* a, String8& dst, String8 args, const char* s) {
    while(*s) {
        string8_push_back(dst, a, *s++);
    }
}

void format_value(Arena* a, String8& dst, String8 args, String8 s) {
    string8_extend(dst, a, s);
}

void format_value(Arena* a, String8& dst, String8 args, int value) {
    char buf[32] = {};
    bool neg = value < 0;
    unsigned int v = neg ? static_cast<unsigned int>(-value) : static_cast<unsigned int>(value);
    int i = 0;
    do {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    } while(v);
    if(neg) buf[i++] = '-';

    for(int j = i - 1; j >= 0; --j) {
        string8_push_back(dst, a, buf[j]);
    }
}

template <class T>
std::enable_if_t<std::is_floating_point_v<T>, void>
format_value(Arena* a, String8& dst, String8 args, T value) {
    u64 int_part = static_cast<u64>(value);
    format_value(a, dst, args, static_cast<int>(int_part));
    string8_push_back(dst, a, '.');
    f64 frac = value - int_part;
    ParseResult<u64> digits = args.slice(1, args.len && args.back() == 'f' ? -2 : -1).parse<u64>();
    u64 n_digits = digits ? min((u64)std::numeric_limits<T>::max_digits10, digits.value) : 3;
    for(u64 i = 0; i < n_digits; ++i) {
        frac *= 10;
        int digit = static_cast<int>(frac);
        string8_push_back(dst, a, static_cast<char>('0' + digit));
        frac -= digit;
    }
}

struct Foo {
    int x;
};

void format_value(Arena* a, String8& dst, String8 args, Foo x) {
    dst.extend(a, S8_LIT("Foo(x="));
    format_value(a, dst, {}, x.x);
    dst.extend(a, S8_LIT(")"));
}

int main(int argc, char* argv[]) {
    Foo f = {};
    println("Hello world: {.12f}, {}, {}", 2.5, 2, f);
}
