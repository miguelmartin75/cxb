#include <cxb/cxb.h>
#include <stdio.h>

template <typename T, typename... Args>
void _format_impl(Arena* a, String8& dst, const char* fmt, const T& first, const Args&... rest) {
    String8 s = {.data = (char*) fmt, .len = 0};

    i64 args_i = -1;
    u64 i = 0;
    while(*fmt) {
        s.len += 1;

        char curr = s[i];
        if(curr == '{') {
            args_i = i;
        } else if(curr == '}') {
            String8 args = s.slice(args_i + 1, i - 1);
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
    String8 dst = {};
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
inline void println(Arena* a, const char* fmt, const Args&... args) {
    auto str = format(a, fmt, args...);
    print("{}\n", str);
}

template <typename... Args>
inline void println(const char* fmt, const Args&... args) {
    auto str = format(fmt, args...);
    print("{}\n", str);
}

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

void format_value(Arena* a, String8& dst, String8 args, double value) {
    long long int_part = static_cast<long long>(value);
    format_value(a, dst, args, static_cast<int>(int_part));
    string8_push_back(dst, a, '.');
    double frac = value - int_part;
    for(int i = 0; i < 6; ++i) {
        frac *= 10;
        int digit = static_cast<int>(frac);
        string8_push_back(dst, a, static_cast<char>('0' + digit));
        frac -= digit;
    }
}

int main(int argc, char* argv[]) {
    println("Hello world: {}, {}", 2.5, 2);
}
