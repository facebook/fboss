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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/switch_asics/Tomahawk3Asic.h"
#include "fboss/agent/hw/switch_asics/Tomahawk4Asic.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/hw/switch_asics/Trident2Asic.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

extern "C" {
#include <bcm/cosq.h>
}

using namespace facebook::fboss;

namespace {
using facebook::fboss::BcmCosQueueStatType;
using facebook::fboss::cfg::QueueCongestionBehavior;
using AqmMap = facebook::fboss::PortQueueFields::AQMMap;

constexpr int kWredDiscardProbability = 100;

const std::map<BcmCosQueueStatType, bcm_cosq_stat_t> kBcmCosqStats = {
    {BcmCosQueueStatType::DROPPED_BYTES, bcmCosqStatDroppedBytes},
    {BcmCosQueueStatType::DROPPED_PACKETS, bcmCosqStatDroppedPackets},
    {BcmCosQueueStatType::OUT_BYTES, bcmCosqStatOutBytes},
    {BcmCosQueueStatType::OUT_PACKETS, bcmCosqStatOutPackets},
    {BcmCosQueueStatType::OBM_LOSSY_HIGH_PRI_DROPPED_PACKETS,
     bcmCosqStatOBMLossyHighPriDroppedPackets},
    {BcmCosQueueStatType::OBM_LOSSY_HIGH_PRI_DROPPED_BYTES,
     bcmCosqStatOBMLossyHighPriDroppedBytes},
    {BcmCosQueueStatType::OBM_LOSSY_LOW_PRI_DROPPED_PACKETS,
     bcmCosqStatOBMLossyLowPriDroppedPackets},
    {BcmCosQueueStatType::OBM_LOSSY_LOW_PRI_DROPPED_BYTES,
     bcmCosqStatOBMLossyLowPriDroppedBytes},
    {BcmCosQueueStatType::OBM_HIGH_WATERMARK, bcmCosqStatOBMHighWatermark},
};

// Default Port Queue settings
// Common settings among different chips
constexpr uint8_t kDefaultPortQueueId = 0;
constexpr auto kDefaultPortQueueScheduling =
    facebook::fboss::cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
constexpr auto kDefaultPortQueueWeight = 1;
const auto kPortQueueNoAqm = AqmMap();

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
  portQueueRate.pktsPerSec_ref() = getRange(minimum, maximum);

  return portQueueRate;
}

AqmMap makeDefauleAqmMap(int32_t threshold) {
  AqmMap aqms;
  facebook::fboss::cfg::LinearQueueCongestionDetection detection;
  detection.minimumLength_ref() = threshold;
  detection.maximumLength_ref() = threshold;
  detection.probability_ref() = kWredDiscardProbability;
  for (auto behavior :
       {QueueCongestionBehavior::EARLY_DROP, QueueCongestionBehavior::ECN}) {
    facebook::fboss::cfg::ActiveQueueManagement aqm;
    aqm.behavior_ref() = behavior;
    aqm.detection_ref()->linear_ref() = detection;
    aqms.emplace(behavior, aqm);
  }
  return aqms;
}
// Unique setting for each single chip
// ======TD2======
// default port queue shared bytes is 8 mmu units
constexpr int kDefaultTD2PortQueueSharedBytes = 1664;
// ======TH======
// default Aqm thresholds
constexpr int32_t kDefaultTHAqmThreshold = 13631280;
const auto kDefaultTHPortQueueAqm = makeDefauleAqmMap(kDefaultTHAqmThreshold);
// default port queue shared bytes is 8 mmu units
constexpr int kDefaultTHPortQueueSharedBytes = 1664;
// ======TH3======
// default port queue shared bytes is 8 mmu units
constexpr int kDefaultTH3PortQueueSharedBytes = 2032;
constexpr int32_t kDefaultTH3AqmThreshold = 13631418;
const auto kDefaultTH3PortQueueAqm = makeDefauleAqmMap(kDefaultTH3AqmThreshold);
// ======TH4======
// TODO(joseph5wu) TH4 default shared bytes is 0, which needs to clarify w/ BRCM
constexpr int kDefaultTH4PortQueueSharedBytes = 0;
// TODO(joseph5wu) It seems like both TH3 and TH4 default aqm threshold is
// 133168898 now. CS00011560232
constexpr int32_t kDefaultTH4AqmThreshold = 13631418;
const auto kDefaultTH4PortQueueAqm = makeDefauleAqmMap(kDefaultTH4AqmThreshold);
} // namespace

namespace facebook::fboss::utility {
bcm_cosq_stat_t getBcmCosqStatType(BcmCosQueueStatType type) {
  return kBcmCosqStats.at(type);
}

const PortQueue& getTD2DefaultUCPortQueueSettings() {
  // Since the default queue is constant, we can use static to cache this
  // object here.
  static Trident2Asic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::UNICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::UNICAST, false /*is front panel port*/),
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
  static TomahawkAsic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::UNICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::UNICAST, false /*is front panel port*/),
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
  static Tomahawk3Asic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::UNICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::UNICAST, false /*is front panel port*/),
      .scalingFactor = bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      .name = std::nullopt,
      .sharedBytes = kDefaultTH3PortQueueSharedBytes,
      .aqms = kDefaultTH3PortQueueAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTH4DefaultUCPortQueueSettings() {
  static Tomahawk4Asic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::UNICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::UNICAST, false /*is front panel port*/),
      .scalingFactor = bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      .name = std::nullopt,
      .sharedBytes = kDefaultTH4PortQueueSharedBytes,
      .aqms = kDefaultTH4PortQueueAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTD2DefaultMCPortQueueSettings() {
  static Trident2Asic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, false /*is front panel port*/),
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
  static TomahawkAsic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, false /*is front panel port*/),
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
  static Tomahawk3Asic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, false /*is front panel port*/),
      .scalingFactor = bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      .name = std::nullopt,
      .sharedBytes = kDefaultTH3PortQueueSharedBytes,
      .aqms = kPortQueueNoAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTH4DefaultMCPortQueueSettings() {
  static Tomahawk4Asic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, false /*is front panel port*/),
      .scalingFactor = bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      .name = std::nullopt,
      .sharedBytes = kDefaultTH4PortQueueSharedBytes,
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
        case BcmChip::TOMAHAWK4:
          return getTH4DefaultUCPortQueueSettings();
      }
    case cfg::StreamType::MULTICAST:
      switch (chip) {
        case BcmChip::TRIDENT2:
          return getTD2DefaultMCPortQueueSettings();
        case BcmChip::TOMAHAWK:
          return getTHDefaultMCPortQueueSettings();
        case BcmChip::TOMAHAWK3:
          return getTH3DefaultMCPortQueueSettings();
        case BcmChip::TOMAHAWK4:
          return getTH4DefaultMCPortQueueSettings();
      }
    case cfg::StreamType::ALL:
      break;
  }
  throw FbossError(
      "Port queue doesn't support streamType:",
      apache::thrift::util::enumNameSafe(streamType));
}

const PortQueue& getTD2DefaultMCCPUQueueSettings() {
  static Trident2Asic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, true /*cpu port*/),
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
  static TomahawkAsic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, true /*cpu port*/),
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
  static Tomahawk3Asic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, true /*cpu port*/),
      .scalingFactor = std::nullopt,
      .name = std::nullopt,
      .sharedBytes = kDefaultTH3PortQueueSharedBytes,
      .aqms = kPortQueueNoAqm,
      .portQueueRate = getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
  }};
  return kPortQueue;
}

const PortQueue& getTH4DefaultMCCPUQueueSettings() {
  static Tomahawk4Asic asic;
  static const PortQueue kPortQueue{PortQueueFields{
      .id = kDefaultPortQueueId,
      .scheduling = kDefaultPortQueueScheduling,
      .streamType = cfg::StreamType::MULTICAST,
      .weight = kDefaultPortQueueWeight,
      .reservedBytes = asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, true /*cpu port*/),
      .scalingFactor = std::nullopt,
      .name = std::nullopt,
      .sharedBytes = kDefaultTH4PortQueueSharedBytes,
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
        case BcmChip::TOMAHAWK4:
          return getTH4DefaultMCCPUQueueSettings();
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

int getMaxCPUQueueSize(int unit) {
  int maxQueueNum = 0;
  auto rv = bcm_rx_queue_max_get(unit, &maxQueueNum);
  bcmCheckError(rv, "failed to get max rx cos queue number");
  // bcm_rx_queue_max_get will return the largest queue id for rx port since
  // CPU usually has more queues than regular port, this function will basically
  // return the leagest queue id for cpu MC queues.
  // As Broadcom SDK use 0 as the first queue, so the max size of rx queues will
  // be maxQueueNum + 1.
  return maxQueueNum + 1;
}
} // namespace facebook::fboss::utility
