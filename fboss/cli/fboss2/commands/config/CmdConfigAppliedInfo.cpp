/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/CmdConfigAppliedInfo.h"
#include <folly/Conv.h>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace facebook::fboss {

namespace {
std::string formatTimestamp(int64_t timestampMs) {
  if (timestampMs == 0) {
    return "Never";
  }

  // Convert milliseconds to seconds
  std::time_t timeInSeconds = timestampMs / 1000;
  int64_t milliseconds = timestampMs % 1000;

  // Convert to local time
  std::tm localTime;
  localtime_r(&timeInSeconds, &localTime);

  // Format the timestamp
  std::ostringstream oss;
  oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
  oss << "." << std::setfill('0') << std::setw(3) << milliseconds;

  return oss.str();
}
} // namespace

CmdConfigAppliedInfoTraits::RetType CmdConfigAppliedInfo::queryClient(
    const HostInfo& hostInfo) {
  auto client =
      utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

  ConfigAppliedInfo configAppliedInfo;
  client->sync_getConfigAppliedInfo(configAppliedInfo);

  return configAppliedInfo;
}

void CmdConfigAppliedInfo::printOutput(const RetType& configAppliedInfo) {
  std::cout << "Config Applied Information:" << std::endl;
  std::cout << "===========================" << std::endl;

  std::cout << "Last Applied Time: "
            << formatTimestamp(*configAppliedInfo.lastAppliedInMs())
            << std::endl;

  std::cout << "Last Coldboot Applied Time: ";
  if (configAppliedInfo.lastColdbootAppliedInMs()) {
    std::cout << formatTimestamp(*configAppliedInfo.lastColdbootAppliedInMs())
              << std::endl;
  } else {
    std::cout << "Not available (warmboot)" << std::endl;
  }
}

} // namespace facebook::fboss
