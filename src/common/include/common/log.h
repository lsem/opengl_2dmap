#pragma once
#include <string>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>


enum class LogMsgType { debug, info, warn, error };

inline void log_impl(LogMsgType type, fmt::string_view format, fmt::format_args args) {
  auto text_style = [](LogMsgType type) {
    switch (type) {
    case LogMsgType::warn:
      return fg(fmt::color::black) | bg(fmt::color::pale_golden_rod);
    case LogMsgType::error:
      return fg(fmt::color::black) | bg(fmt::color::tomato);
    case LogMsgType::debug:
      return fg(fmt::color::light_gray);

    default:
      return fmt::text_style{};
    }
    return fmt::text_style{};
  };
  auto label = [](LogMsgType type) {
    switch (type) {
    case LogMsgType::warn:
      return "warn";
    case LogMsgType::error:
      return "error";
    case LogMsgType::info:
      return "info";
    case LogMsgType::debug:
      return "debug";
    }
    return "";
  };
  auto format_str = fmt::format("{}: {}", label(type), format);
  std::cout << fmt::vformat(text_style(type), std::move(format_str), args)
            << "\n";
}

template <typename... Args>
inline void log_warn(fmt::string_view format, Args... args) {
  log_impl(LogMsgType::warn, format,
           fmt::make_args_checked<Args...>(format, args...));
}
template <typename... Args>
inline void log_err(fmt::string_view format, Args... args) {
  log_impl(LogMsgType::error, format, fmt::make_args_checked<Args...>(format, args...));
}

template <typename... Args>
inline void log_debug(fmt::string_view format, Args... args) {
  log_impl(LogMsgType::debug, format, fmt::make_args_checked<Args...>(format, args...));
}

