// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FsdbHelper.h"

namespace facebook::fboss {

std::vector<std::string> fsdbAgentDataSwitchStateRootPath() {
  return {};
}

std::vector<std::string> switchStateRootPath() {
  return {};
}

void printOperDeltaPaths(const fsdb::OperDelta& /*operDelta*/) {}

std::string getOperPath(const std::vector<std::string>& /*tokens*/) {
  return "";
}
} // namespace facebook::fboss
