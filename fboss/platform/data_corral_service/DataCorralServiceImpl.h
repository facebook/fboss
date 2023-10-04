/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <folly/Synchronized.h>
#include <folly/experimental/FunctionScheduler.h>

#include "fboss/platform/data_corral_service/ChassisManager.h"
#include "fboss/platform/data_corral_service/FruPresenceExplorer.h"
#include "fboss/platform/data_corral_service/if/gen-cpp2/data_corral_service_types.h"

namespace facebook::fboss::platform::data_corral_service {

struct SensorLiveData {
  float value;
  int64_t timeStamp;
};

class DataCorralServiceImpl {
 public:
  DataCorralServiceImpl() {
    init();
  }
  explicit DataCorralServiceImpl(const std::string& platformName)
      : platformName_{platformName} {
    init();
  }

  std::vector<FruIdData> getFruid(bool uncached);

 private:
  // Designate platform name
  std::string platformName_{};

  // Cached Fruid
  std::vector<std::pair<std::string, std::string>> fruid_{};
  std::unique_ptr<ChassisManager> chassisManager_;
  std::shared_ptr<FruPresenceExplorer> fruPresenceExplorer_;
  folly::FunctionScheduler presenceDetectionScheduler_;
  void init();
};

} // namespace facebook::fboss::platform::data_corral_service
