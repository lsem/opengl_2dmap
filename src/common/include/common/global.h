#pragma once

#include "gg/gg.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <tuple>
#include <filesystem>

#include "span.hpp"

using std::string;
using std::string_view;
using std::unique_ptr;
using std::vector;
using std::tuple;

using nonstd::span;

using gg::p32;
using gg::v2;

namespace fs = std::filesystem;