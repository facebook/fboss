// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class BroadcomAsic : public HwAsic {
 public:
  using HwAsic::HwAsic;
  std::string getVendor() const override {
    return "bcm";
  }
  uint32_t getMaxMirrors() const override {
    return 4;
  }
  uint32_t getSflowShimHeaderSize() const override {
    return 8;
  }
  std::optional<uint32_t> getPortSerdesPreemphasis() const override {
    return std::nullopt;
  }
  AsicVendor getAsicVendor() const override {
    return HwAsic::AsicVendor::ASIC_VENDOR_BCM;
  }
  uint32_t getNumCores() const override {
    throw FbossError("Num cores API not supported");
  }
  bool scalingFactorBasedDynamicThresholdSupported() const override {
    return true;
  }
  int getBufferDynThreshFromScalingFactor(
      cfg::MMUScalingFactor scalingFactor) const override {
    switch (scalingFactor) {
      case cfg::MMUScalingFactor::ONE:
        return 0;
      case cfg::MMUScalingFactor::EIGHT:
        return 3;
      case cfg::MMUScalingFactor::ONE_128TH:
        return -7;
      case cfg::MMUScalingFactor::ONE_64TH:
        return -6;
      case cfg::MMUScalingFactor::ONE_32TH:
        return -5;
      case cfg::MMUScalingFactor::ONE_16TH:
        return -4;
      case cfg::MMUScalingFactor::ONE_8TH:
        return -3;
      case cfg::MMUScalingFactor::ONE_QUARTER:
        return -2;
      case cfg::MMUScalingFactor::ONE_HALF:
        return -1;
      case cfg::MMUScalingFactor::TWO:
        return 1;
      case cfg::MMUScalingFactor::FOUR:
        return 2;
      case cfg::MMUScalingFactor::ONE_32768:
        // Unsupported
        throw FbossError(
            "Unsupported scaling factor : ",
            apache::thrift::util::enumNameSafe(scalingFactor));
    }
    CHECK(0) << "Should never get here";
    return -1;
  }
};
} // namespace facebook::fboss
