// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/BeasAsic.h"
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
bool BeasAsic::isSupported(Feature feature) const {
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
      return true;
    default:
      return false;
  }
  return false;
}

std::set<cfg::StreamType> BeasAsic::getQueueStreamTypes(
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
      "Beas Asic does not support port type: ",
      apache::thrift::util::enumNameSafe(portType));
}

int BeasAsic::getDefaultNumPortQueues(
    cfg::StreamType /* streamType */,
    bool cpu) const {
  // On kamet we use a single fabric queue for all
  // traffic
  return cpu ? 0 : 1;
}
uint64_t BeasAsic::getDefaultReservedBytes(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Beas doesn't support queue feature");
}
cfg::MMUScalingFactor BeasAsic::getDefaultScalingFactor(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Beas doesn't support queue feature");
}

uint32_t BeasAsic::getMaxMirrors() const {
  return 0;
}
uint16_t BeasAsic::getMirrorTruncateSize() const {
  throw FbossError("Beas doesn't support mirror feature");
}

uint32_t BeasAsic::getMaxLabelStackDepth() const {
  throw FbossError("Beas doesn't support label feature");
};
uint64_t BeasAsic::getMMUSizeBytes() const {
  throw FbossError("Beas doesn't support MMU feature");
};
int BeasAsic::getMaxNumLogicalPorts() const {
  throw FbossError("Beas doesn't support logical ports feature");
}
uint32_t BeasAsic::getMaxWideEcmpSize() const {
  throw FbossError("Beas doesn't support ecmp feature");
}
uint32_t BeasAsic::getMaxLagMemberSize() const {
  throw FbossError("Beas doesn't support lag feature");
}
uint32_t BeasAsic::getPacketBufferUnitSize() const {
  throw FbossError("Beas doesn't support MMU feature");
}
uint32_t BeasAsic::getPacketBufferDescriptorSize() const {
  throw FbossError("Beas doesn't support MMU feature");
}
uint32_t BeasAsic::getMaxVariableWidthEcmpSize() const {
  return 512;
}
uint32_t BeasAsic::getMaxEcmpSize() const {
  return 4096;
}

uint32_t BeasAsic::getNumCores() const {
  throw FbossError("Num cores API not supported by BeasAsic");
}

bool BeasAsic::scalingFactorBasedDynamicThresholdSupported() const {
  throw FbossError("Beas doesn't support MMU feature");
}
int BeasAsic::getBufferDynThreshFromScalingFactor(
    cfg::MMUScalingFactor /* scalingFactor */) const {
  throw FbossError("Beas doesn't support MMU feature");
}
uint32_t BeasAsic::getStaticQueueLimitBytes() const {
  throw FbossError("Beas doesn't support MMU feature");
}
uint32_t BeasAsic::getNumMemoryBuffers() const {
  throw FbossError("Beas doesn't support MMU feature");
}
}; // namespace facebook::fboss
