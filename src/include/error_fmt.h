#pragma once
#include <fmt/core.h>
#include <fmt/color.h>

#define ERROR_FMT(val) fmt::styled(val, fmt::emphasis::bold | fmt::fg(fmt::terminal_color::red))
#define WARN_FMT(val) fmt::styled(val, fmt::emphasis::bold | fmt::fg(fmt::terminal_color::yellow))
#define VERBOSE_FMT(val) fmt::styled(val, fmt::emphasis::bold | fmt::fg(fmt::terminal_color::cyan))
#define INFO_FMT(val) fmt::styled(val, fmt::emphasis::bold | fmt::fg(fmt::terminal_color::green))

#define VALIDATION_FMT(val) fmt::styled(val, fmt::emphasis::bold | fmt::fg(fmt::terminal_color::magenta))
#define PERFORMANCE_FMT(val) fmt::styled(val, fmt::emphasis::bold | fmt::fg(fmt::terminal_color::yellow))
#define GENERAL_FMT(val) fmt::styled(val, fmt::emphasis::bold | fmt::fg(fmt::terminal_color::blue))
