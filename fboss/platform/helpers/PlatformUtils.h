// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace facebook::fboss::platform {

class PlatformUtils {
 public:
  virtual ~PlatformUtils() = default;
  // Executes command and returns exit status and standard output
  virtual std::pair<int, std::string> execCommand(const std::string& cmd) const;
};

} // namespace facebook::fboss::platform
