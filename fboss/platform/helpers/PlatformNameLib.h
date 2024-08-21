// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>

#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::helpers {

class PlatformNameLib {
 public:
  auto static constexpr dmidecodeCommand = "dmidecode -s system-product-name";
  auto static constexpr kPlatformNameBiosReads = "platform_name_bios_reads";
  auto static constexpr kPlatformNameBiosReadFailures =
      "platform_name_bios_read_failures";

  virtual ~PlatformNameLib() = default;

  // Returns platform name from dmidecode. Platform names are UPPERCASE but
  // otherwise should match config names. Known dmidecode values are mapped to
  // config names.
  virtual std::string getPlatformNameFromBios(
      const PlatformUtils& platformUtils = PlatformUtils()) const;

  // Returns platform name matching getPlatformNameFromBios, but (1) allows
  // forcing a name via parameter and (2) returns nullopt instead of throwing
  // exceptions.
  virtual std::optional<std::string> getPlatformName(
      const PlatformUtils& platformUtils = PlatformUtils()) const;
};

} // namespace facebook::fboss::platform::helpers
