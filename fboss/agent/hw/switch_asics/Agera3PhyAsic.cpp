// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/switch_asics/Agera3PhyAsic.h"
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
bool Agera3PhyAsic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::MACSEC:
    case HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT:
    case HwAsic::Feature::EMPTY_ACL_MATCHER:
    case HwAsic::Feature::PORT_EYE_VALUES:
    case HwAsic::Feature::FEC:
    case HwAsic::Feature::XPHY_PORT_STATE_TOGGLE:
    case HwAsic::Feature::PMD_RX_SIGNAL_DETECT:
    case HwAsic::Feature::PMD_RX_LOCK_STATUS:
    case HwAsic::Feature::FEC_AM_LOCK_STATUS:
    case HwAsic::Feature::PCS_RX_LINK_STATUS:
    case HwAsic::Feature::WARMBOOT:
    case HwAsic::Feature::OBJECT_KEY_CACHE:
    case HwAsic::Feature::ARS_ALTERNATE_MEMBERS:
      return false;
    default:
      return false;
  }
}

std::set<cfg::StreamType> Agera3PhyAsic::getQueueStreamTypes(
    cfg::PortType /*portType*/) const {
  throw FbossError("Agera3PhyAsic doesn't support queue feature");
}
int Agera3PhyAsic::getDefaultNumPortQueues(
    cfg::StreamType /* streamType */,
    cfg::PortType /*portType*/) const {
  throw FbossError("Agera3PhyAsic doesn't support queue feature");
}
std::optional<uint64_t> Agera3PhyAsic::getDefaultReservedBytes(
    cfg::StreamType /* streamType */,
    cfg::PortType /* portType */) const {
  throw FbossError("Agera3PhyAsic doesn't support queue feature");
}
std::optional<cfg::MMUScalingFactor> Agera3PhyAsic::getDefaultScalingFactor(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Agera3PhyAsic doesn't support queue feature");
}

uint32_t Agera3PhyAsic::getMaxMirrors() const {
  throw FbossError("Agera3PhyAsic doesn't support mirror feature");
}
uint16_t Agera3PhyAsic::getMirrorTruncateSize() const {
  throw FbossError("Agera3PhyAsic doesn't support mirror feature");
}

uint32_t Agera3PhyAsic::getMaxLabelStackDepth() const {
  throw FbossError("Agera3PhyAsic doesn't support label feature");
};
uint64_t Agera3PhyAsic::getMMUSizeBytes() const {
  throw FbossError("Agera3PhyAsic doesn't support MMU feature");
};
uint64_t Agera3PhyAsic::getSramSizeBytes() const {
  throw FbossError("Agera3PhyAsic doesn't support MMU feature");
}
int Agera3PhyAsic::getMaxNumLogicalPorts() const {
  throw FbossError("Agera3PhyAsic doesn't support logical ports feature");
}
uint32_t Agera3PhyAsic::getMaxWideEcmpSize() const {
  throw FbossError("Agera3PhyAsic doesn't support ecmp feature");
}
uint32_t Agera3PhyAsic::getMaxLagMemberSize() const {
  throw FbossError("Agera3PhyAsic doesn't support lag feature");
}
uint32_t Agera3PhyAsic::getPacketBufferUnitSize() const {
  throw FbossError("Agera3PhyAsic doesn't support MMU feature");
}
uint32_t Agera3PhyAsic::getPacketBufferDescriptorSize() const {
  throw FbossError("Agera3PhyAsic doesn't support MMU feature");
}

uint32_t Agera3PhyAsic::getMaxVariableWidthEcmpSize() const {
  throw FbossError("Agera3PhyAsic doesn't support ecmp feature");
}
uint32_t Agera3PhyAsic::getMaxEcmpSize() const {
  throw FbossError("Agera3PhyAsic doesn't support ecmp feature");
}

uint32_t Agera3PhyAsic::getNumCores() const {
  throw FbossError("Num cores API not supported");
}

bool Agera3PhyAsic::scalingFactorBasedDynamicThresholdSupported() const {
  throw FbossError("Agera3PhyAsic doesn't support MMU feature");
}
int Agera3PhyAsic::getBufferDynThreshFromScalingFactor(
    cfg::MMUScalingFactor /* scalingFactor */) const {
  throw FbossError("Agera3PhyAsic doesn't support MMU feature");
}
uint32_t Agera3PhyAsic::getStaticQueueLimitBytes() const {
  throw FbossError("Agera3PhyAsic doesn't support MMU feature");
}
uint32_t Agera3PhyAsic::getNumMemoryBuffers() const {
  throw FbossError("Agera3PhyAsic doesn't support MMU feature");
}
int Agera3PhyAsic::getMidPriCpuQueueId() const {
  throw FbossError("Agera3PhyAsic does not support cpu queue");
}
int Agera3PhyAsic::getHiPriCpuQueueId() const {
  throw FbossError("Agera3PhyAsic does not support cpu queue");
}
std::optional<uint32_t> Agera3PhyAsic::getMaxArsGroups() const {
  return std::nullopt;
}

std::optional<uint32_t> Agera3PhyAsic::getArsBaseIndex() const {
  return std::nullopt;
}
}; // namespace facebook::fboss
