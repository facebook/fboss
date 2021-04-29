// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/switch_asics/Elbert8DDAsic.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
bool Elbert8DDAsic::isSupported(Feature feature) const {
  switch (feature) {
    case HwAsic::Feature::MACSEC:
      return true;
    default:
      return false;
  }
  return false;
}

std::set<cfg::StreamType> Elbert8DDAsic::getQueueStreamTypes(
    bool /* cpu */) const {
  throw FbossError("Elbert8DDAsic doesn't support queue feature");
}
int Elbert8DDAsic::getDefaultNumPortQueues(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Elbert8DDAsic doesn't support queue feature");
}
uint64_t Elbert8DDAsic::getDefaultReservedBytes(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Elbert8DDAsic doesn't support queue feature");
}
cfg::MMUScalingFactor Elbert8DDAsic::getDefaultScalingFactor(
    cfg::StreamType /* streamType */,
    bool /* cpu */) const {
  throw FbossError("Elbert8DDAsic doesn't support queue feature");
}

uint32_t Elbert8DDAsic::getMaxMirrors() const {
  throw FbossError("Elbert8DDAsic doesn't support mirror feature");
}
uint16_t Elbert8DDAsic::getMirrorTruncateSize() const {
  throw FbossError("Elbert8DDAsic doesn't support mirror feature");
}

uint32_t Elbert8DDAsic::getMaxLabelStackDepth() const {
  throw FbossError("Elbert8DDAsic doesn't support label feature");
};
uint64_t Elbert8DDAsic::getMMUSizeBytes() const {
  throw FbossError("Elbert8DDAsic doesn't support MMU feature");
};
int Elbert8DDAsic::getMaxNumLogicalPorts() const {
  throw FbossError("Elbert8DDAsic doesn't support logical ports feature");
}
uint32_t Elbert8DDAsic::getMaxWideEcmpSize() const {
  throw FbossError("Elbert8DDAsic doesn't support ecmp feature");
}
}; // namespace facebook::fboss
