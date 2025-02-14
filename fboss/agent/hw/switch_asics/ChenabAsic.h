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
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class ChenabAsic : public HwAsic {
 public:
  ChenabAsic(
      std::optional<int64_t> switchId,
      cfg::SwitchInfo switchInfo,
      std::optional<cfg::SdkVersion> sdkVersion = std::nullopt)
      : HwAsic(switchId, switchInfo, sdkVersion, {cfg::SwitchType::NPU}) {}

  AsicVendor getAsicVendor() const override;
  std::string getVendor() const override;
  bool isSupported(Feature feature) const override;
  cfg::AsicType getAsicType() const override;
  int getBufferDynThreshFromScalingFactor(
      cfg::MMUScalingFactor /* scalingFactor */) const override;
  bool scalingFactorBasedDynamicThresholdSupported() const override;
  phy::DataPlanePhyChipType getDataPlanePhyChipType() const override;
  cfg::PortSpeed getMaxPortSpeed() const override;
  std::set<cfg::StreamType> getQueueStreamTypes(
      cfg::PortType portType) const override;
  int getDefaultNumPortQueues(
      cfg::StreamType streamType,
      cfg::PortType /*portType*/) const override;
  uint32_t getMaxLabelStackDepth() const override;
  uint64_t getMMUSizeBytes() const override;
  uint64_t getSramSizeBytes() const override;
  uint32_t getMaxMirrors() const override;
  std::optional<uint64_t> getDefaultReservedBytes(
      cfg::StreamType /*streamType*/,
      cfg::PortType /*portType*/) const override;
  std::optional<cfg::MMUScalingFactor> getDefaultScalingFactor(
      cfg::StreamType /*streamType*/,
      bool /*cpu*/) const override;
  int getMaxNumLogicalPorts() const override;
  uint16_t getMirrorTruncateSize() const override;
  uint32_t getMaxWideEcmpSize() const override;
  uint32_t getMaxLagMemberSize() const override;
  int getSflowPortIDOffset() const override;
  uint32_t getSflowShimHeaderSize() const override;
  std::optional<uint32_t> getPortSerdesPreemphasis() const override;
  uint32_t getPacketBufferUnitSize() const override;
  uint32_t getPacketBufferDescriptorSize() const override;
  uint32_t getMaxVariableWidthEcmpSize() const override;
  uint32_t getMaxEcmpSize() const override;
  uint32_t getNumCores() const override;
  uint32_t getStaticQueueLimitBytes() const override;
  uint32_t getNumMemoryBuffers() const override;
  uint16_t getGreProtocol() const override;
  cfg::Range64 getReservedEncapIndexRange() const override;
  int getMidPriCpuQueueId() const override;
  int getHiPriCpuQueueId() const override;

 private:
  bool isSupportedFabric(Feature feature) const;
  bool isSupportedNonFabric(Feature feature) const;
};

} // namespace facebook::fboss
