#pragma once

#include "gg/gg.h"
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "span.hpp"
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

using std::optional;
using std::string;
using std::string_view;
using std::tuple;
using std::unique_ptr;
using std::vector;

using nonstd::span;

using gg::p32;
using gg::v2;

namespace fs = std::filesystem;

using namespace std::chrono_literals;

using steady_clock = std::chrono::steady_clock;
