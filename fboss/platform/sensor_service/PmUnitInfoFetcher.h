// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <array>
#include <optional>
#include <string>

namespace facebook::fboss::platform::sensor_service {
class PmUnitInfoFetcher {
 public:
  virtual ~PmUnitInfoFetcher() = default;
  // Returns {ProductProductionState,ProductVersion,ProductSubVersion}
  // TODO: Need to figure out how we're going to fetch (thrift? file? manual
  // eeprom reading?)
  virtual std::optional<std::array<int16_t, 3>> fetch(
      const std::string& slotPath,
      const std::string& pmUnitName) const;
};

} // namespace facebook::fboss::platform::sensor_service
