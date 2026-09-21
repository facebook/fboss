/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/WarmBootFileUtils.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/SysError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

void WarmBootFileUtils::setCanWarmBoot(const std::string& flagPath) {
  auto updateFd =
      creat(flagPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (updateFd < 0) {
    throw SysError(errno, "Unable to create ", flagPath);
  }
  close(updateFd);
  XLOG(DBG1) << "Wrote can warm boot flag: " << flagPath;
}

state::SwitchState WarmBootFileUtils::getWarmBootThriftState(
    const std::string& filePath) {
  state::SwitchState thriftState;
  if (!readThriftFromBinaryFile(filePath, thriftState)) {
    throw FbossError("Failed to read thrift state from ", filePath);
  }
  return thriftState;
}

void WarmBootFileUtils::storeWarmBootThriftState(
    const std::string& filePath,
    const state::SwitchState& switchThriftState) {
  auto rc = dumpBinaryThriftToFile(filePath, switchThriftState);
  if (!rc) {
    XLOG(FATAL) << "Error while storing switch state to thrift state file: "
                << filePath;
  }
}

std::shared_ptr<SwitchState> WarmBootFileUtils::reconstructWarmBootThriftState(
    const std::optional<state::SwitchState>& warmBootState) {
  std::shared_ptr<SwitchState> state{nullptr};
  if (warmBootState.has_value()) {
    state = SwitchState::fromThrift(*(warmBootState));
  }
  return state;
}

state::WarmbootState WarmBootFileUtils::getWarmBootState(
    const std::string& filePath) {
  state::WarmbootState thriftState;
  if (!readThriftFromBinaryFile(filePath, thriftState)) {
    throw FbossError("Failed to read thrift state from ", filePath);
  }
  return thriftState;
}

void WarmBootFileUtils::storeWarmBootState(
    const std::string& filePath,
    const state::WarmbootState& switchStateThrift) {
  auto rc = dumpBinaryThriftToFile(filePath, switchStateThrift);
  if (!rc) {
    XLOG(FATAL) << "Error while storing switch state to thrift state file: "
                << filePath;
  }
}

} // namespace facebook::fboss
