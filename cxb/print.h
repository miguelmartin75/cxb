#ifndef CXB_PRINT_H
#define CXB_PRINT_H

// TODO(lowpri): remove fmtlib dependencies
#include <cxb/cxb.h>

#include <fmt/core.h>
#include <fmt/ranges.h>

// https://github.com/fmtlib/fmt/issues/2134
template <typename... T>
void print(fmt::format_string<T...> fmt, T&&... args) {
    fmt::print(fmt, forward<T>(args)...);
    fflush(stdout);
}

template <typename... T>
void print_nf(fmt::format_string<T...> fmt, T&&... args) {
    fmt::print(fmt, forward<T>(args)...);
    fflush(stdout);
}

template <typename... T>
void println(fmt::format_string<T...> fmt, T&&... args) {
    auto str = fmt::format(fmt, forward<T>(args)...);
    print("{}\n", str);
}

template <typename... T>
void println_nf(fmt::format_string<T...> fmt, T&&... args) {
    auto str = fmt::format(fmt, forward<T>(args)...);
    print("{}\n", str);
}

template<>
struct fmt::formatter<StringSlice>
{
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
      return ctx.begin();
  }

  template<typename FormatContext>
  auto format(const StringSlice& x, FormatContext& ctx) const {
      return fmt::format_to(ctx.out(), "StringSlice(\"{:.{}}\", n={})", x.data, x.len, x.len);
  }
};

#endif
