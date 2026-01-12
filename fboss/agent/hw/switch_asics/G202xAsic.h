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

class G202xAsic : public TajoAsic {
 public:
  G202xAsic(
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
    return cfg::AsicType::ASIC_TYPE_G202X;
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
    // 128 physical lanes + cpu
    return 129;
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
     * G202x supports ~20K next hop group(NHG) members, but we are limiting it
     * to 9088 for now. Reason: ASIC has two level ECMP, where:
     *
     * Level 1 has:
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
     * But the test passed for 18172 and failed for 18173. So, returning 18172.
     * Will work with Cisco to understand the reason for this. - MT-803
     */
    return 18172;
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

  std::optional<uint32_t> getMaxNdpTableSize() const override {
    return 512;
  }

  std::optional<uint32_t> getMaxArpTableSize() const override {
    return 512;
  }

  std::optional<uint32_t> getMaxUnifiedNeighborTableSize() const override {
    return 512;
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
