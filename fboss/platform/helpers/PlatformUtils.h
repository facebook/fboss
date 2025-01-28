// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

namespace facebook::fboss::platform {

class PlatformUtils {
 public:
  virtual ~PlatformUtils() = default;
  // Executes command and returns exit status and standard output
  virtual std::pair<int, std::string> execCommand(const std::string& cmd) const;
  virtual std::pair<int, std::string> runCommand(
      const std::vector<std::string>& args,
      bool = false) const;
  virtual std::pair<int, std::string> runCommandWithStdin(
      const std::vector<std::string>&,
      const std::string&,
      bool = false) const;
};

} // namespace facebook::fboss::platform
