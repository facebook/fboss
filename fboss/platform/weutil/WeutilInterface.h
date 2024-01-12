// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace facebook::fboss::platform {

class WeutilInterface {
 public:
  WeutilInterface() {}
  virtual ~WeutilInterface() = default;

  virtual std::vector<std::pair<std::string, std::string>> getContents() = 0;
  virtual void printInfo() = 0;
  virtual void printInfoJson() = 0;
};

} // namespace facebook::fboss::platform
