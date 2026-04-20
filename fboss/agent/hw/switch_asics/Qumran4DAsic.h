// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <optional>
#include <utility>
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"

namespace facebook::fboss {

class Qumran4DAsic : public Jericho3Asic {
 public:
  Qumran4DAsic(
      std::optional<int64_t> switchId,
      cfg::SwitchInfo switchInfo,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt)
      : Jericho3Asic(switchId, std::move(switchInfo), std::move(sdkVersion)) {}

  bool isSupported(Feature) const override;

  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_QUMRAN4D;
  }

  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::EIGHTHUNDREDG;
  }

  uint32_t getNumCores() const override {
    return 8;
  }

  uint32_t getNumDies() const override {
    return 2;
  }
};

} // namespace facebook::fboss
