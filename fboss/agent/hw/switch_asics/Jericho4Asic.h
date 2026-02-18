// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <optional>
#include "fboss/agent/hw/switch_asics/Q4DAsic.h"

namespace facebook::fboss {

class Jericho4Asic : public Q4DAsic {
 public:
  Jericho4Asic(
      std::optional<int64_t> switchId,
      cfg::SwitchInfo switchInfo,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt)
      : Q4DAsic(switchId, switchInfo, sdkVersion) {}

  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_JERICHO4;
  }
};

} // namespace facebook::fboss
