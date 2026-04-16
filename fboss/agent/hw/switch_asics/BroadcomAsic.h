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
  std::vector<prbs::PrbsPolynomial> getSupportedPrbsPolynomials()
      const override {
    return {
        prbs::PrbsPolynomial::PRBS7,
        prbs::PrbsPolynomial::PRBS15,
        prbs::PrbsPolynomial::PRBS23,
        prbs::PrbsPolynomial::PRBS31,
        prbs::PrbsPolynomial::PRBS9,
        prbs::PrbsPolynomial::PRBS11,
        prbs::PrbsPolynomial::PRBS58};
  }
  std::optional<uint32_t> getMaxArsGroups() const override {
    return std::nullopt;
  }
  std::optional<uint32_t> getArsBaseIndex() const override {
    return std::nullopt;
  }
};
} // namespace facebook::fboss
