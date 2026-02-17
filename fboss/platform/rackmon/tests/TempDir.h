// Copyright 2022-present Facebook. All Rights Reserved.
#include <sys/stat.h>
#include <filesystem>
#include <sstream>

namespace rackmon {
class TempDirectory {
  std::string path_{};

 public:
  TempDirectory() {
    char mkdir_template[] = "/tmp/rackmonXXXXXX";
    path_.assign(mkdtemp(mkdir_template));
  }
  ~TempDirectory() {
    std::filesystem::remove_all(path_);
  }
  const std::string& path() const {
    return path_;
  }
};
} // namespace rackmon
