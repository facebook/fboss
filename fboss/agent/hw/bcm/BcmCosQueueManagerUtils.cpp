/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"

#include "fboss/agent/hw/bcm/BcmCosQueueFBConvertors.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

using namespace facebook::fboss;

namespace {
using facebook::fboss::BcmCosQueueStatType;
using facebook::fboss::cfg::QueueCongestionBehavior;
using AqmMap = facebook::fboss::PortQueueFields::AQMMap;

const std::map<BcmCosQueueStatType, bcm_cosq_stat_t> kBcmCosqStats = {
    {BcmCosQueueStatType::DROPPED_BYTES, bcmCosqStatDroppedBytes},
    {BcmCosQueueStatType::DROPPED_PACKETS, bcmCosqStatDroppedPackets},
    {BcmCosQueueStatType::OUT_BYTES, bcmCosqStatOutBytes},
    {BcmCosQueueStatType::OUT_PACKETS, bcmCosqStatOutPackets}};

// Default Port Queue settings
// Common settings among different chips
constexpr uint8_t kDefaultPortQueueId = 0;
constexpr auto kDefaultPortQueueScheduling =
    facebook::fboss::cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
constexpr auto kDefaultPortQueueWeight = 1;
const auto kPortQueueNoAqm = AqmMap();

constexpr int kDefaultPortQueueReservedBytes = 0;
constexpr bcm_cosq_control_drop_limit_alpha_value_t kDefaultPortQueueAlpha =
    bcmCosqControlDropLimitAlpha_2;

constexpr int kDefaultPortQueuePacketsPerSecMin = 0;
constexpr int kDefaultPortQueuePacketsPerSecMax = 0;

cfg::Range getRange(uint32_t minimum, uint32_t maximum) {
  cfg::Range range;
  range.minimum_ref() = minimum;
  range.maximum_ref() = maximum;

  return range;
}

cfg::PortQueueRate getPortQueueRatePps(uint32_t minimum, uint32_t maximum) {
  cfg::PortQueueRate portQueueRate;
  portQueueRate.set_pktsPerSec(getRange(minimum, maximum));

  return portQueueRate;
}

AqmMap makeDefauleAqmMap(int32_t threshold) {
  AqmMap aqms;
  facebook::fboss::cfg::LinearQueueCongestionDetection detection;
  detection.minimumLength = detection.maximumLength = threshold;
  for (auto behavior :
       {QueueCongestionBehavior::EARLY_DROP, QueueCongestionBehavior::ECN}) {
    facebook::fboss::cfg::ActiveQueueManagement aqm;
    aqm.behavior = behavior;
    aqm.detection.set_linear(detection);
    aqms.emplace(behavior, aqm);
  }
  return aqms;
}
// Unique setting for each single chip
// ======TD2======
// default cpu queue control bytes is 8 mmu units
constexpr int kDefaultTD2CPUQueueReservedBytes = 1664;
// default port queue shared bytes is 8 mmu units
constexpr int kDefaultTD2PortQueueSharedBytes = 1664;
// ======TH======
// default cpu queue control bytes is 8 mmu units
constexpr int kDefaultTHCPUQueueReservedBytes = 1664;
// default Aqm thresholds
constexpr int32_t kDefaultTHAqmThreshold = 13631280;
const auto kDefaultTHPortQueueAqm = makeDefauleAqmMap(kDefaultTHAqmThreshold);
}; // namespace
// default port queue shared bytes is 8 mmu units
constexpr int kDefaultTHPortQueueSharedBytes = 1664;
// ======TH3======
// default cpu queue control bytes is 7 mmu units
constexpr int kDefaultTH3CPUQueueReservedBytes = 1778;
// default port queue shared bytes is 8 mmu units
constexpr int kDefaultTH3PortQueueSharedBytes = 2032;
constexpr int32_t kDefaultTH3AqmThreshold = 13631418;
const auto kDefaultTH3PortQueueAqm = makeDefauleAqmMap(kDefaultTH3AqmThreshold);

namespace facebook::fboss::utility {
bcm_cosq_stat_t getBcmCosqStatType(BcmCosQueueStatType type) {
  return kBcmCosqStats.at(type);
}

const PortQueue& getTD2DefaultUCPortQueueSettings() {
  // Since the default queue is constant, we can use static to cache this
  // object here.
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::UNICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = kDefaultPortQueueReservedBytes,
      .scalingFactor = bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      .name = std::nullopt,
      .sharedBytes = kDefaultTD2PortQueueSharedBytes,
      .aqms = kPortQueueNoAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTHDefaultUCPortQueueSettings() {
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::UNICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = kDefaultPortQueueReservedBytes,
      .scalingFactor = bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      .name = std::nullopt,
      .sharedBytes = kDefaultTHPortQueueSharedBytes,
      .aqms = kDefaultTHPortQueueAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTH3DefaultUCPortQueueSettings() {
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::UNICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = kDefaultPortQueueReservedBytes,
      .scalingFactor = bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      .name = std::nullopt,
      .sharedBytes = kDefaultTH3PortQueueSharedBytes,
      .aqms = kDefaultTH3PortQueueAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTD2DefaultMCPortQueueSettings() {
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = kDefaultPortQueueReservedBytes,
      .scalingFactor = bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      .name = std::nullopt,
      .sharedBytes = kDefaultTD2PortQueueSharedBytes,
      .aqms = kPortQueueNoAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTHDefaultMCPortQueueSettings() {
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = kDefaultPortQueueReservedBytes,
      .scalingFactor = bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      .name = std::nullopt,
      .sharedBytes = kDefaultTHPortQueueSharedBytes,
      .aqms = kPortQueueNoAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTH3DefaultMCPortQueueSettings() {
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = kDefaultPortQueueReservedBytes,
      .scalingFactor = bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      .name = std::nullopt,
      .sharedBytes = kDefaultTH3PortQueueSharedBytes,
      .aqms = kPortQueueNoAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getDefaultPortQueueSettings(
    BcmChip chip,
    cfg::StreamType streamType) {
  switch (streamType) {
    case cfg::StreamType::UNICAST:
      switch (chip) {
        case BcmChip::TRIDENT2:
          return getTD2DefaultUCPortQueueSettings();
        case BcmChip::TOMAHAWK:
          return getTHDefaultUCPortQueueSettings();
        case BcmChip::TOMAHAWK3:
          return getTH3DefaultUCPortQueueSettings();
      }
    case cfg::StreamType::MULTICAST:
      switch (chip) {
        case BcmChip::TRIDENT2:
          return getTD2DefaultMCPortQueueSettings();
        case BcmChip::TOMAHAWK:
          return getTHDefaultMCPortQueueSettings();
        case BcmChip::TOMAHAWK3:
          return getTH3DefaultMCPortQueueSettings();
      }
    case cfg::StreamType::ALL:
      break;
  }
  throw FbossError(
      "Port queue doesn't support streamType:",
      apache::thrift::util::enumNameSafe(streamType));
}

const PortQueue& getTD2DefaultMCCPUQueueSettings() {
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = kDefaultTD2CPUQueueReservedBytes,
      .scalingFactor = std::nullopt,
      .name = std::nullopt,
      .sharedBytes = kDefaultTD2PortQueueSharedBytes,
      .aqms = kPortQueueNoAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTHDefaultMCCPUQueueSettings() {
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = kDefaultTHCPUQueueReservedBytes,
      .scalingFactor = std::nullopt,
      .name = std::nullopt,
      .sharedBytes = kDefaultTHPortQueueSharedBytes,
      .aqms = kPortQueueNoAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTH3DefaultMCCPUQueueSettings() {
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = kDefaultTH3CPUQueueReservedBytes,
      .scalingFactor = std::nullopt,
      .name = std::nullopt,
      .sharedBytes = kDefaultTH3PortQueueSharedBytes,
      .aqms = kPortQueueNoAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getDefaultControlPlaneQueueSettings(
    BcmChip chip,
    cfg::StreamType streamType) {
  switch (streamType) {
    case cfg::StreamType::MULTICAST:
      switch (chip) {
        case BcmChip::TRIDENT2:
          return getTD2DefaultMCCPUQueueSettings();
        case BcmChip::TOMAHAWK:
          return getTHDefaultMCCPUQueueSettings();
        case BcmChip::TOMAHAWK3:
          return getTH3DefaultMCCPUQueueSettings();
      }
    case cfg::StreamType::UNICAST:
    case cfg::StreamType::ALL:
      throw FbossError("Control Plane doesn't support unicast queue");
  }
  throw FbossError(
      "Control Plane queue doesn't support streamType:",
      apache::thrift::util::enumNameSafe(streamType));
  ;
}

} // namespace facebook::fboss::utility
