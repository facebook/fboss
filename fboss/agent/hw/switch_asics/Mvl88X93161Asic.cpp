// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/Mvl88X93161Asic.h"
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
bool Mvl88X93161Asic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT:
    case HwAsic::Feature::FEC:
    case HwAsic::Feature::SAI_PORT_SERDES_FIELDS_RESET:
    case HwAsic::Feature::PORT_INTERFACE_TYPE:
      return true;
    default:
      return false;
  }
  return false;
}

std::set<cfg::StreamType> Mvl88X93161Asic::getQueueStreamTypes(
    bool /* cpu */) const {
  throw FbossError("Mvl88X93161Asic doesn't support queue feature");
}
int Mvl88X93161Asic::getDefaultNumPortQueues(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Mvl88X93161Asic doesn't support queue feature");
}
uint64_t Mvl88X93161Asic::getDefaultReservedBytes(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Mvl88X93161Asic doesn't support queue feature");
}
cfg::MMUScalingFactor Mvl88X93161Asic::getDefaultScalingFactor(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Mvl88X93161Asic doesn't support queue feature");
}

uint32_t Mvl88X93161Asic::getMaxMirrors() const {
  throw FbossError("Mvl88X93161Asic doesn't support mirror feature");
}
uint16_t Mvl88X93161Asic::getMirrorTruncateSize() const {
  throw FbossError("Mvl88X93161Asic doesn't support mirror feature");
}

uint32_t Mvl88X93161Asic::getMaxLabelStackDepth() const {
  throw FbossError("Mvl88X93161Asic doesn't support label feature");
};
uint64_t Mvl88X93161Asic::getMMUSizeBytes() const {
  throw FbossError("Mvl88X93161Asic doesn't support MMU feature");
};
int Mvl88X93161Asic::getMaxNumLogicalPorts() const {
  throw FbossError("Mvl88X93161Asic doesn't support logical ports feature");
}
uint32_t Mvl88X93161Asic::getMaxWideEcmpSize() const {
  throw FbossError("Mvl88X93161Asic doesn't support ecmp feature");
}
uint32_t Mvl88X93161Asic::getMaxLagMemberSize() const {
  throw FbossError("Mvl88X93161Asic doesn't support lag feature");
}
uint32_t Mvl88X93161Asic::getPacketBufferUnitSize() const {
  throw FbossError("Mvl88X93161Asic doesn't support MMU feature");
}
uint32_t Mvl88X93161Asic::getPacketBufferDescriptorSize() const {
  throw FbossError("Mvl88X93161Asic doesn't support MMU feature");
}
uint32_t Mvl88X93161Asic::getMaxVariableWidthEcmpSize() const {
  return 512;
}
uint32_t Mvl88X93161Asic::getMaxEcmpSize() const {
  return 4096;
}

uint32_t Mvl88X93161Asic::getNumCores() const {
  throw FbossError("Num cores API not supported");
}

bool Mvl88X93161Asic::scalingFactorBasedDynamicThresholdSupported() const {
  throw FbossError("Mvl88X93161Asic doesn't support MMU feature");
}
int Mvl88X93161Asic::getBufferDynThreshFromScalingFactor(
    cfg::MMUScalingFactor /* scalingFactor */) const {
  throw FbossError("Mvl88X93161Asic doesn't support MMU feature");
}
uint32_t Mvl88X93161Asic::getStaticQueueLimitBytes() const {
  throw FbossError("Mvl88X93161Asic doesn't support MMU feature");
}
}; // namespace facebook::fboss
