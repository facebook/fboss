// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <optional>
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"

namespace facebook::fboss {

class Q4DAsic : public Jericho3Asic {
 public:
  Q4DAsic(
      std::optional<int64_t> switchId,
      cfg::SwitchInfo switchInfo,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt)
      : Jericho3Asic(switchId, switchInfo, sdkVersion) {}

  bool isSupported(Feature) const override;

  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_Q4D;
  }

  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::EIGHTHUNDREDG;
  }

  uint32_t getNumCores() const override {
    return 8;
  }
};

} // namespace facebook::fboss
