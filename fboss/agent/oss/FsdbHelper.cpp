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

} // namespace facebook::fboss
