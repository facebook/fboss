// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <vector>

namespace facebook::fboss::platform {

class ContentValidator {
 public:
  bool isValid(
      const std::vector<std::pair<std::string, std::string>>& contents);
};

} // namespace facebook::fboss::platform
