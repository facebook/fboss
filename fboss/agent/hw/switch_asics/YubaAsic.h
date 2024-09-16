/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/TajoAsic.h"

namespace facebook::fboss {

class YubaAsic : public TajoAsic {
 public:
  YubaAsic(
      cfg::SwitchType type,
      std::optional<int64_t> id,
      int16_t index,
      std::optional<cfg::Range64> systemPortRange,
      const folly::MacAddress& mac,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt)
      : TajoAsic(
            type,
            id,
            index,
            systemPortRange,
            mac,
            sdkVersion,
            {cfg::SwitchType::NPU,
             cfg::SwitchType::VOQ,
             cfg::SwitchType::FABRIC}) {
    HwAsic::setDefaultStreamType(cfg::StreamType::UNICAST);
  }
  bool isSupported(Feature feature) const override {
    return getSwitchType() != cfg::SwitchType::FABRIC
        ? isSupportedNonFabric(feature)
        : isSupportedFabric(feature);
  }
  cfg::AsicType getAsicType() const override {
    return cfg::AsicType::ASIC_TYPE_YUBA;
  }
  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override {
    return phy::DataPlanePhyChipType::IPHY;
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::EIGHTHUNDREDG;
  }
  std::set<cfg::StreamType> getQueueStreamTypes(
      cfg::PortType portType) const override;
  int getDefaultNumPortQueues(
      cfg::StreamType streamType,
      cfg::PortType /*portType*/) const override;
  uint32_t getMaxLabelStackDepth() const override {
    return 3;
  }
  uint64_t getMMUSizeBytes() const override {
    return 256 * 1024 * 1024;
  }
  uint32_t getMaxMirrors() const override {
    // TODO - verify this
    return 4;
  }
  uint64_t getDefaultReservedBytes(
      cfg::StreamType /*streamType*/,
      cfg::PortType /*portType*/) const override {
    // Concept of reserved bytes does not apply to GB
    return 0;
  }
  cfg::MMUScalingFactor getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    // Concept of scaling factor does not apply returning the same value TH3
    return cfg::MMUScalingFactor::TWO;
  }
  int getMaxNumLogicalPorts() const override {
    // 256 physical lanes + cpu
    return 257;
  }
  uint16_t getMirrorTruncateSize() const override {
    return 220;
  }
  uint32_t getMaxWideEcmpSize() const override {
    return 128;
  }
  uint32_t getMaxLagMemberSize() const override {
    return 512;
  }
  int getSystemPortIDOffset() const override {
    return 0;
  }
  uint32_t getSflowShimHeaderSize() const override {
    return 9;
  }
  std::optional<uint32_t> getPortSerdesPreemphasis() const override {
    return 50;
  }
  uint32_t getPacketBufferUnitSize() const override {
    return 512;
  }
  uint32_t getPacketBufferDescriptorSize() const override {
    return 40;
  }
  uint32_t getMaxVariableWidthEcmpSize() const override {
    return 512;
  }
  uint32_t getMaxEcmpSize() const override {
    return 512;
  }
  uint32_t getNumCores() const override {
    return 12;
  }
  uint32_t getStaticQueueLimitBytes() const override {
    return 512 * 1024 * getPacketBufferUnitSize();
  }
  uint32_t getNumMemoryBuffers() const override {
    return 1;
  }
  cfg::Range64 getReservedEncapIndexRange() const override;

  std::vector<prbs::PrbsPolynomial> getSupportedPrbsPolynomials()
      const override {
    return {
        prbs::PrbsPolynomial::PRBS9,
        prbs::PrbsPolynomial::PRBS11,
        prbs::PrbsPolynomial::PRBS13,
        prbs::PrbsPolynomial::PRBS15,
        prbs::PrbsPolynomial::PRBS31,
    };
  }

 private:
  bool isSupportedFabric(Feature feature) const;
  bool isSupportedNonFabric(Feature feature) const;
};

} // namespace facebook::fboss
