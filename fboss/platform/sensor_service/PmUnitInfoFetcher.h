// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <array>
#include <optional>
#include <string>

#include "fboss/platform/sensor_service/PmClientFactory.h"

namespace facebook::fboss::platform::sensor_service {
class PmUnitInfoFetcher {
 public:
  explicit PmUnitInfoFetcher(
      const std::shared_ptr<PmClientFactory> pmClientFactory =
          std::make_shared<PmClientFactory>());
  virtual ~PmUnitInfoFetcher() = default;
  // Returns {ProductProductionState,ProductVersion,ProductSubVersion}
  // TODO: Need to figure out how we're going to fetch (thrift? file? manual
  // eeprom reading?)
  virtual std::optional<std::array<int16_t, 3>> fetch(
      const std::string& slotPath) const;

 private:
  std::shared_ptr<PmClientFactory> pmClientFactory_;
};

} // namespace facebook::fboss::platform::sensor_service
