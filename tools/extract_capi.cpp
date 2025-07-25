#include <cxb/cxb.h>

#include <stdio.h>

template <typename T, typename... Args>
void _format_impl(Arena* a, StringSlice& dst, const char* fmt, const T& first, const Args&... rest) {
    StringSlice s = {
        .data = (char*)fmt,
        .len = 0
    };

    i64 args_i = -1;
    u64 i = 0;
    while(*fmt) {
        s.len += 1;

        char curr = s[i];
        if (curr == '{') {
            args_i = i;
        } else if(curr == '}') {
            StringSlice args = s.slice(args_i + 1, i - 1);
            format_value(a, dst, args, first);
            _format_impl(a, dst, s.data + i + 1, rest...);
            break;
        } else if(args_i < 0) {
            push_back(a, dst, curr);
        }
        ++i;
        ++fmt;
    }
}

void _format_impl(Arena* a, StringSlice& dst, const char* fmt) { while (*fmt) push_back(a, dst, *fmt++); }

template <typename... Args>
StringSlice format(Arena* a, const char* fmt, const Args&... args) {
    StringSlice dst = {};
    _format_impl(a, dst, fmt, args...);
    return dst;
}

template <typename... Args>
StringSlice format(const char* fmt, const Args&... args) {
    Arena* a = arena_make_nbytes(KB(1));  // TODO: scratch buffer
    return format(a, fmt, args...);
}


template <typename... Args>
void print(FILE* f, Arena* a, const char* fmt, const Args&... args) {
    StringSlice str = format(a, fmt, args...);
    if(str.data) {
        fwrite(&str[0], sizeof(char), str.len, f);
    }
}

template <typename... Args>
void print(FILE* f, const char* fmt, const Args&... args) {
    StringSlice str = format(fmt, args...);
    if(str.data) {
        fwrite(&str[0], sizeof(char), str.len, f);
    }
}

template <typename... Args> inline void print(const char* fmt, const Args&... args) { 
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

void format_value(Arena* a, StringSlice& dst, StringSlice args, const char* s) {
    while (*s) {
        push_back(a, dst, *s++);
    }
}

void format_value(Arena* a, StringSlice& dst, StringSlice args, StringSlice s) {
    extend(a, dst, s);
}

void format_value(Arena* a, StringSlice& dst, StringSlice args, int value) {
    char buf[32] = {};
    bool neg = value < 0;
    unsigned int v = neg ? static_cast<unsigned int>(-value) : static_cast<unsigned int>(value);
    int i = 0;
    do {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    } while (v);
    if (neg) buf[i++] = '-';

    for (int j = i - 1; j >= 0; --j) {
        push_back(a, dst, buf[j]);
    }
}

void format_value(Arena* a, StringSlice& dst, StringSlice args, double value) {
    long long int_part = static_cast<long long>(value);
    format_value(a, dst, args, static_cast<int>(int_part));
    push_back(a, dst, '.');
    double frac = value - int_part;
    for (int i = 0; i < 6; ++i) {
        frac *= 10;
        int digit = static_cast<int>(frac);
        push_back(a, dst, static_cast<char>('0' + digit));
        frac -= digit;
    }
}

int main(int argc, char* argv[]) {
    println("Hello world: {}, {}", 2.5, 2);
}
