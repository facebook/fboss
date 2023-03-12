// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/RamonAsic.h"
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
bool RamonAsic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::REMOVE_PORTS_FOR_COLDBOOT:
    case HwAsic::Feature::FABRIC_PORTS:
    case HwAsic::Feature::LINK_TRAINING:
    case HwAsic::Feature::SAI_PORT_SERDES_FIELDS_RESET:
    case HwAsic::Feature::FABRIC_TX_QUEUES:
    case HwAsic::Feature::PMD_RX_LOCK_STATUS:
    case HwAsic::Feature::PMD_RX_SIGNAL_DETECT:
    case HwAsic::Feature::FEC:
    case HwAsic::Feature::FEC_CORRECTED_BITS:
    case HwAsic::Feature::PORT_FABRIC_ISOLATE:
      return true;
    default:
      return false;
  }
  return false;
}

std::set<cfg::StreamType> RamonAsic::getQueueStreamTypes(
    cfg::PortType portType) const {
  switch (portType) {
    case cfg::PortType::CPU_PORT:
    case cfg::PortType::INTERFACE_PORT:
    case cfg::PortType::RECYCLE_PORT:
      break;
    case cfg::PortType::FABRIC_PORT:
      return {cfg::StreamType::FABRIC_TX};
  }
  throw FbossError(
      "Ramon Asic does not support port type: ",
      apache::thrift::util::enumNameSafe(portType));
}

int RamonAsic::getDefaultNumPortQueues(
    cfg::StreamType /* streamType */,
    bool cpu) const {
  // On meru400bfu we use a single fabric queue for all
  // traffic
  return cpu ? 0 : 1;
}
uint64_t RamonAsic::getDefaultReservedBytes(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Ramon doesn't support queue feature");
}
cfg::MMUScalingFactor RamonAsic::getDefaultScalingFactor(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Ramon doesn't support queue feature");
}

uint32_t RamonAsic::getMaxMirrors() const {
  return 0;
}
uint16_t RamonAsic::getMirrorTruncateSize() const {
  throw FbossError("Ramon doesn't support mirror feature");
}

uint32_t RamonAsic::getMaxLabelStackDepth() const {
  throw FbossError("Ramon doesn't support label feature");
};
uint64_t RamonAsic::getMMUSizeBytes() const {
  throw FbossError("Ramon doesn't support MMU feature");
};
int RamonAsic::getMaxNumLogicalPorts() const {
  throw FbossError("Ramon doesn't support logical ports feature");
}
uint32_t RamonAsic::getMaxWideEcmpSize() const {
  throw FbossError("Ramon doesn't support ecmp feature");
}
uint32_t RamonAsic::getMaxLagMemberSize() const {
  throw FbossError("Ramon doesn't support lag feature");
}
uint32_t RamonAsic::getPacketBufferUnitSize() const {
  throw FbossError("Ramon doesn't support MMU feature");
}
uint32_t RamonAsic::getPacketBufferDescriptorSize() const {
  throw FbossError("Ramon doesn't support MMU feature");
}
uint32_t RamonAsic::getMaxVariableWidthEcmpSize() const {
  return 512;
}
uint32_t RamonAsic::getMaxEcmpSize() const {
  return 4096;
}

uint32_t RamonAsic::getNumCores() const {
  throw FbossError("Num cores API not supported by RamonAsic");
}

bool RamonAsic::scalingFactorBasedDynamicThresholdSupported() const {
  throw FbossError("Ramon doesn't support MMU feature");
}
int RamonAsic::getBufferDynThreshFromScalingFactor(
    cfg::MMUScalingFactor /* scalingFactor */) const {
  throw FbossError("Ramon doesn't support MMU feature");
}
uint32_t RamonAsic::getStaticQueueLimitBytes() const {
  throw FbossError("Ramon doesn't support MMU feature");
}
uint32_t RamonAsic::getNumMemoryBuffers() const {
  throw FbossError("Ramon doesn't support MMU feature");
}
}; // namespace facebook::fboss
