#!/usr/bin/env bash
find src -name "*.h" -o -name "*.cpp" -exec clang-format -i -dry-run {} \;

