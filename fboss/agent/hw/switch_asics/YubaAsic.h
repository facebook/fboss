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
      std::optional<int64_t> switchId,
      cfg::SwitchInfo switchInfo,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt)
      : TajoAsic(switchId, switchInfo, sdkVersion, {cfg::SwitchType::NPU}) {
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
  uint64_t getSramSizeBytes() const override {
    // No HBM!
    return getMMUSizeBytes();
  }
  uint32_t getMaxMirrors() const override {
    // TODO - verify this
    return 4;
  }
  std::optional<uint64_t> getDefaultReservedBytes(
      cfg::StreamType /*streamType*/,
      cfg::PortType /*portType*/) const override {
    // Concept of reserved bytes does not apply to GB
    return 0;
  }
  std::optional<cfg::MMUScalingFactor> getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override {
    // Concept of scaling factor does not apply returning the same value TH3
    return cfg::MMUScalingFactor::TWO;
  }
  const std::map<cfg::PortType, cfg::PortLoopbackMode>& desiredLoopbackModes()
      const override;
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
  int getSflowPortIDOffset() const override {
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
  std::optional<uint32_t> getMaxEcmpGroups() const override {
    return 1024;
  }
  std::optional<uint32_t> getMaxEcmpMembers() const override {
    /*
     * G200 supports ~20K next hop group(NHG) members, but we are limiting it to
     * 9088 for now. Reason: ASIC has two level ECMP, where: Level 1 has:
     *        1. 512 NHG/ECMP groups
     *        2. 10240 - 1024 (Service port GID limit)
     *                 - 128 (max size of ECMP group) = 9088 NHG members
     *
     * Level 2 has:
     *        1. 512 NHG/ECMP groups
     *        2. 10240 - 512 (IP hosts limit)
     *                 - 512 (Next hop GID limit)
     *                 - 128 (max size of ECMP group) = 9088 NHG members
     * Together:
     *        1. 1024 NHG/ECMP groups
     *        2. 9088 + 9088 = 18,176 NHG/ECMP members = ~18K
     *
     * Constraint:
     *        We cannot have less than 512 ECMP group and all 18K members,
     *        because we will still be in only level 1. Hence if number of ECMP
     *        groups created is less than 512, max ECMP members available will
     *        be 9088.
     *
     * Keeping the above constraint in mind, we can use the below formula to
     * calculate max ECMP members during resource accounting:
     *
     * Let X  := current_group_count
     * Let Y  := current_member_count
     * Let Y' := Y + new_add_count
     * Let L  := (X / 513) + 1 --> Level
     * Let MT := 0.75 * L * 9088 --> member Threshold
     * On receiving a request to create a new NHG member:
     *     if Y' > MT:
     *         reject
     *     else:
     *         add
     *
     * But for now, we will limit it to 9088.
     * As cisco provides a fix to remove the constraint, we will just retun
     * 18,176.
     */
    return 9088;
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

  cfg::IpTunnelMode getTunnelDscpMode() const override {
    return cfg::IpTunnelMode::UNIFORM;
  }

 private:
  bool isSupportedFabric(Feature feature) const;
  bool isSupportedNonFabric(Feature feature) const;
};

} // namespace facebook::fboss
