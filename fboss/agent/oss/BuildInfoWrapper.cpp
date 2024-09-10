// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/BuildInfoWrapper.h"

#include <string>

namespace facebook::fboss {

std::string getBuildPackageVersion() {
  return std::string("buildPackageVersion");
}

} // namespace facebook::fboss
