// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::helpers {

class PlatformNameLib {
 public:
  auto static constexpr dmidecodeCommand = "dmidecode -s system-product-name";

  virtual ~PlatformNameLib() = default;
  virtual std::string getPlatformNameFromBios(
      const PlatformUtils& platformUtils = PlatformUtils()) const;
};

} // namespace facebook::fboss::platform::helpers
