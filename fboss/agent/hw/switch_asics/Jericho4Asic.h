// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <optional>
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"

namespace facebook::fboss {

class Jericho4Asic : public Jericho3Asic {
 public:
  Jericho4Asic(
      std::optional<int64_t> switchId,
      cfg::SwitchInfo switchInfo,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt)
      : Jericho3Asic(switchId, switchInfo, sdkVersion) {}

  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_JERICHO4;
  }

  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::ONEPOINTSIXT;
  }

  bool isSupported(Feature feature) const override {
    // TBD if necessary
    if (feature == HwAsic::Feature::SWITCH_ISOLATE) {
      return false;
    }
    return Jericho3Asic::isSupported(feature);

  uint32_t getNumCores() const override {
    return 8;
  }
};

} // namespace facebook::fboss
