// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/MarvelPhyAsic.h"
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
bool MarvelPhyAsic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT:
    case HwAsic::Feature::FEC:
    case HwAsic::Feature::EXTENDED_FEC:
    case HwAsic::Feature::SAI_PORT_SERDES_FIELDS_RESET:
    case HwAsic::Feature::PORT_INTERFACE_TYPE:
    case HwAsic::Feature::XPHY_PORT_STATE_TOGGLE:
      return true;
    // TODO(rajank):
    // Enable PHY Warmboot once tested
    case HwAsic::Feature::WARMBOOT:
      return false;
    default:
      return false;
  }
  return false;
}

std::set<cfg::StreamType> MarvelPhyAsic::getQueueStreamTypes(
    cfg::PortType /*portType*/) const {
  throw FbossError("MarvelPhyAsic doesn't support queue feature");
}
int MarvelPhyAsic::getDefaultNumPortQueues(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("MarvelPhyAsic doesn't support queue feature");
}
uint64_t MarvelPhyAsic::getDefaultReservedBytes(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("MarvelPhyAsic doesn't support queue feature");
}
cfg::MMUScalingFactor MarvelPhyAsic::getDefaultScalingFactor(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("MarvelPhyAsic doesn't support queue feature");
}

uint32_t MarvelPhyAsic::getMaxMirrors() const {
  throw FbossError("MarvelPhyAsic doesn't support mirror feature");
}
uint16_t MarvelPhyAsic::getMirrorTruncateSize() const {
  throw FbossError("MarvelPhyAsic doesn't support mirror feature");
}

uint32_t MarvelPhyAsic::getMaxLabelStackDepth() const {
  throw FbossError("MarvelPhyAsic doesn't support label feature");
};
uint64_t MarvelPhyAsic::getMMUSizeBytes() const {
  throw FbossError("MarvelPhyAsic doesn't support MMU feature");
};
int MarvelPhyAsic::getMaxNumLogicalPorts() const {
  throw FbossError("MarvelPhyAsic doesn't support logical ports feature");
}
uint32_t MarvelPhyAsic::getMaxWideEcmpSize() const {
  throw FbossError("MarvelPhyAsic doesn't support ecmp feature");
}
uint32_t MarvelPhyAsic::getMaxLagMemberSize() const {
  throw FbossError("MarvelPhyAsic doesn't support lag feature");
}
uint32_t MarvelPhyAsic::getPacketBufferUnitSize() const {
  throw FbossError("MarvelPhyAsic doesn't support MMU feature");
}
uint32_t MarvelPhyAsic::getPacketBufferDescriptorSize() const {
  throw FbossError("MarvelPhyAsic doesn't support MMU feature");
}
uint32_t MarvelPhyAsic::getMaxVariableWidthEcmpSize() const {
  return 512;
}
uint32_t MarvelPhyAsic::getMaxEcmpSize() const {
  return 4096;
}

uint32_t MarvelPhyAsic::getNumCores() const {
  throw FbossError("Num cores API not supported");
}

bool MarvelPhyAsic::scalingFactorBasedDynamicThresholdSupported() const {
  throw FbossError("MarvelPhyAsic doesn't support MMU feature");
}
int MarvelPhyAsic::getBufferDynThreshFromScalingFactor(
    cfg::MMUScalingFactor /* scalingFactor */) const {
  throw FbossError("MarvelPhyAsic doesn't support MMU feature");
}
uint32_t MarvelPhyAsic::getStaticQueueLimitBytes() const {
  throw FbossError("MarvelPhyAsic doesn't support MMU feature");
}
uint32_t MarvelPhyAsic::getNumMemoryBuffers() const {
  throw FbossError("MarvelPhyAsic doesn't support MMU feature");
}
}; // namespace facebook::fboss
