// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/BeasAsic.h"
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
bool Beas::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT:
      return true;
    default:
      return false;
  }
  return false;
}

std::set<cfg::StreamType> Beas::getQueueStreamTypes(bool /* cpu */) const {
  throw FbossError("Beas doesn't support queue feature");
}
int Beas::getDefaultNumPortQueues(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Beas doesn't support queue feature");
}
uint64_t Beas::getDefaultReservedBytes(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Beas doesn't support queue feature");
}
cfg::MMUScalingFactor Beas::getDefaultScalingFactor(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Beas doesn't support queue feature");
}

uint32_t Beas::getMaxMirrors() const {
  throw FbossError("Beas doesn't support mirror feature");
}
uint16_t Beas::getMirrorTruncateSize() const {
  throw FbossError("Beas doesn't support mirror feature");
}

uint32_t Beas::getMaxLabelStackDepth() const {
  throw FbossError("Beas doesn't support label feature");
};
uint64_t Beas::getMMUSizeBytes() const {
  throw FbossError("Beas doesn't support MMU feature");
};
int Beas::getMaxNumLogicalPorts() const {
  throw FbossError("Beas doesn't support logical ports feature");
}
uint32_t Beas::getMaxWideEcmpSize() const {
  throw FbossError("Beas doesn't support ecmp feature");
}
uint32_t Beas::getMaxLagMemberSize() const {
  throw FbossError("Beas doesn't support lag feature");
}
uint32_t Beas::getPacketBufferUnitSize() const {
  throw FbossError("Beas doesn't support MMU feature");
}
uint32_t Beas::getPacketBufferDescriptorSize() const {
  throw FbossError("Beas doesn't support MMU feature");
}
uint32_t Beas::getMaxVariableWidthEcmpSize() const {
  return 512;
}
uint32_t Beas::getMaxEcmpSize() const {
  return 4096;
}

uint32_t Beas::getNumCores() const {
  throw FbossError("Num cores API not supported");
}

bool Beas::scalingFactorBasedDynamicThresholdSupported() const {
  throw FbossError("Beas doesn't support MMU feature");
}
int Beas::getBufferDynThreshFromScalingFactor(
    cfg::MMUScalingFactor /* scalingFactor */) const {
  throw FbossError("Beas doesn't support MMU feature");
}
uint32_t Beas::getStaticQueueLimitBytes() const {
  throw FbossError("Beas doesn't support MMU feature");
}
}; // namespace facebook::fboss
