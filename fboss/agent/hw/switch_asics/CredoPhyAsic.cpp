// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/CredoPhyAsic.h"
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
bool CredoPhyAsic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::MACSEC:
    case HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT:
    case HwAsic::Feature::EMPTY_ACL_MATCHER:
    case HwAsic::Feature::PORT_EYE_VALUES:
    case HwAsic::Feature::FEC:
    case HwAsic::Feature::XPHY_PORT_STATE_TOGGLE:
      return true;
    default:
      return false;
  }
  return false;
}

std::set<cfg::StreamType> CredoPhyAsic::getQueueStreamTypes(
    cfg::PortType /*portType*/) const {
  throw FbossError("CredoPhyAsic doesn't support queue feature");
}
int CredoPhyAsic::getDefaultNumPortQueues(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("CredoPhyAsic doesn't support queue feature");
}
uint64_t CredoPhyAsic::getDefaultReservedBytes(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("CredoPhyAsic doesn't support queue feature");
}
cfg::MMUScalingFactor CredoPhyAsic::getDefaultScalingFactor(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("CredoPhyAsic doesn't support queue feature");
}

uint32_t CredoPhyAsic::getMaxMirrors() const {
  throw FbossError("CredoPhyAsic doesn't support mirror feature");
}
uint16_t CredoPhyAsic::getMirrorTruncateSize() const {
  throw FbossError("CredoPhyAsic doesn't support mirror feature");
}

uint32_t CredoPhyAsic::getMaxLabelStackDepth() const {
  throw FbossError("CredoPhyAsic doesn't support label feature");
};
uint64_t CredoPhyAsic::getMMUSizeBytes() const {
  throw FbossError("CredoPhyAsic doesn't support MMU feature");
};
int CredoPhyAsic::getMaxNumLogicalPorts() const {
  throw FbossError("CredoPhyAsic doesn't support logical ports feature");
}
uint32_t CredoPhyAsic::getMaxWideEcmpSize() const {
  throw FbossError("CredoPhyAsic doesn't support ecmp feature");
}
uint32_t CredoPhyAsic::getMaxLagMemberSize() const {
  throw FbossError("CredoPhyAsic doesn't support lag feature");
}
uint32_t CredoPhyAsic::getPacketBufferUnitSize() const {
  throw FbossError("CredoPhyAsic doesn't support MMU feature");
}
uint32_t CredoPhyAsic::getPacketBufferDescriptorSize() const {
  throw FbossError("CredoPhyAsic doesn't support MMU feature");
}
uint32_t CredoPhyAsic::getMaxVariableWidthEcmpSize() const {
  return 512;
}
uint32_t CredoPhyAsic::getMaxEcmpSize() const {
  return 4096;
}

uint32_t CredoPhyAsic::getNumCores() const {
  throw FbossError("Num cores API not supported");
}

bool CredoPhyAsic::scalingFactorBasedDynamicThresholdSupported() const {
  throw FbossError("CredoPhyAsic doesn't support MMU feature");
}
int CredoPhyAsic::getBufferDynThreshFromScalingFactor(
    cfg::MMUScalingFactor /* scalingFactor */) const {
  throw FbossError("CredoPhyAsic doesn't support MMU feature");
}
uint32_t CredoPhyAsic::getStaticQueueLimitBytes() const {
  throw FbossError("CredoPhyAsic doesn't support MMU feature");
}
uint32_t CredoPhyAsic::getNumMemoryBuffers() const {
  throw FbossError("CredoPhyAsic doesn't support MMU feature");
}
}; // namespace facebook::fboss
