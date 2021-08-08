#pragma once

#include <filesystem>

#ifdef __linux__
#include <limits.h>
#include <unistd.h>
#endif // __linux__

namespace os {
// todo: make it returning Expeted<fpath_t>
inline std::filesystem::path get_exe_path() {
#ifdef __linux__
  char result[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
  if (count > 0) {
    return std::string{result, static_cast<size_t>(count)};
  } else {
    return {};
  }
#endif
  // todo: use GetModuleFileNameW on wnindows.
}
} // namespace os