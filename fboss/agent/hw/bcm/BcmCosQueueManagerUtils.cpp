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
#include <optional>

extern "C" {
#include <bcm/cosq.h>
}

using namespace facebook::fboss;

namespace {
using facebook::fboss::BcmCosQueueStatType;
using facebook::fboss::cfg::QueueCongestionBehavior;
using AqmMap = facebook::fboss::PortQueue::AQMMap;

constexpr int kWredDiscardProbability = 100;

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
  range.minimum() = minimum;
  range.maximum() = maximum;

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
  detection.minimumLength() = threshold;
  detection.maximumLength() = threshold;
  detection.probability() = kWredDiscardProbability;
  for (auto behavior :
       {QueueCongestionBehavior::EARLY_DROP, QueueCongestionBehavior::ECN}) {
    facebook::fboss::cfg::ActiveQueueManagement aqm;
    aqm.behavior() = behavior;
    aqm.detection()->linear_ref() = detection;
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

PortQueueFields getPortQueueFields(
    uint8_t id,
    cfg::QueueScheduling scheduling,
    cfg::StreamType streamType,
    int weight,
    std::optional<int> reservedBytes,
    std::optional<cfg::MMUScalingFactor> scalingFactor,
    std::optional<std::string> name,
    std::optional<int> sharedBytes,
    PortQueue::AQMMap aqms,
    std::optional<cfg::PortQueueRate> portQueueRate,
    std::optional<int> bandwidthBurstMinKbits,
    std::optional<int> bandwidthBurstMaxKbits,
    std::optional<TrafficClass> trafficClass,
    std::optional<std::set<PfcPriority>> pfcPriorities) {
  PortQueueFields queue;
  *queue.id() = id;
  *queue.weight() = weight;
  if (reservedBytes) {
    queue.reserved() = reservedBytes.value();
  }
  if (scalingFactor) {
    auto scalingFactorName = apache::thrift::util::enumName(*scalingFactor);
    if (scalingFactorName == nullptr) {
      CHECK(false) << "Unexpected MMU scaling factor: "
                   << static_cast<int>(*scalingFactor);
    }
    queue.scalingFactor() = scalingFactorName;
  }
  auto schedulingName = apache::thrift::util::enumName(scheduling);
  if (schedulingName == nullptr) {
    CHECK(false) << "Unexpected scheduling: " << static_cast<int>(scheduling);
  }
  *queue.scheduling() = schedulingName;
  auto streamTypeName = apache::thrift::util::enumName(streamType);
  if (streamTypeName == nullptr) {
    CHECK(false) << "Unexpected streamType: " << static_cast<int>(streamType);
  }
  *queue.streamType() = streamTypeName;
  if (name) {
    queue.name() = name.value();
  }
  if (sharedBytes) {
    queue.sharedBytes() = sharedBytes.value();
  }
  if (!aqms.empty()) {
    std::vector<cfg::ActiveQueueManagement> aqmList;
    for (const auto& aqm : aqms) {
      aqmList.push_back(aqm.second);
    }
    queue.aqms() = aqmList;
  }

  if (portQueueRate) {
    queue.portQueueRate() = portQueueRate.value();
  }

  if (bandwidthBurstMinKbits) {
    queue.bandwidthBurstMinKbits() = bandwidthBurstMinKbits.value();
  }

  if (bandwidthBurstMaxKbits) {
    queue.bandwidthBurstMaxKbits() = bandwidthBurstMaxKbits.value();
  }

  if (trafficClass) {
    queue.trafficClass() = static_cast<int16_t>(trafficClass.value());
  }

  if (pfcPriorities) {
    std::vector<int16_t> pfcPris;
    for (const auto& pfcPriority : pfcPriorities.value()) {
      pfcPris.push_back(static_cast<int16_t>(pfcPriority));
    }
    queue.pfcPriorities() = pfcPris;
  }

  return queue;
}
} // namespace

namespace facebook::fboss::utility {
bcm_cosq_stat_t getBcmCosqStatType(
    BcmCosQueueStatType type,
    cfg::AsicType asic) {
  static std::map<BcmCosQueueStatType, bcm_cosq_stat_t> cosqStats{};
  if (cosqStats.size() == 0) {
    std::map<BcmCosQueueStatType, bcm_cosq_stat_t> bcmCosqStats = {
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

    if (asic == cfg::AsicType::ASIC_TYPE_TOMAHAWK3 ||
        asic == cfg::AsicType::ASIC_TYPE_TOMAHAWK4) {
      bcmCosqStats[BcmCosQueueStatType::WRED_DROPPED_PACKETS] =
          bcmCosqStatTotalDiscardDroppedPackets;
    } else if (asic == cfg::AsicType::ASIC_TYPE_TOMAHAWK) {
      bcmCosqStats[BcmCosQueueStatType::WRED_DROPPED_PACKETS] =
          bcmCosqStatDiscardUCDroppedPackets;
    }
    cosqStats = bcmCosqStats;
  }

  return cosqStats.at(type);
}

const PortQueue& getTD2DefaultUCPortQueueSettings() {
  // Since the default queue is constant, we can use static to cache this
  // object here.
  folly::MacAddress mac;
  static Trident2Asic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::UNICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::UNICAST, false /*is front panel port*/),
      bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      std::nullopt,
      kDefaultTD2PortQueueSharedBytes,
      kPortQueueNoAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
  return kPortQueue;
}

const PortQueue& getTHDefaultUCPortQueueSettings() {
  folly::MacAddress mac;
  static TomahawkAsic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::UNICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::UNICAST, false /*is front panel port*/),
      bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      std::nullopt,
      kDefaultTHPortQueueSharedBytes,
      kDefaultTHPortQueueAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
  return kPortQueue;
}

const PortQueue& getTH3DefaultUCPortQueueSettings() {
  folly::MacAddress mac;
  static Tomahawk3Asic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::UNICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::UNICAST, false /*is front panel port*/),
      bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      std::nullopt,
      kDefaultTH3PortQueueSharedBytes,
      kDefaultTH3PortQueueAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
  return kPortQueue;
}

const PortQueue& getTH4DefaultUCPortQueueSettings() {
  folly::MacAddress mac;
  static Tomahawk4Asic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::UNICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::UNICAST, false /*is front panel port*/),
      bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      std::nullopt,
      kDefaultTH4PortQueueSharedBytes,
      kDefaultTH4PortQueueAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
  return kPortQueue;
}

const PortQueue& getTD2DefaultMCPortQueueSettings() {
  folly::MacAddress mac;
  static Trident2Asic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::MULTICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, false /*is front panel port*/),
      bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      std::nullopt,
      kDefaultTD2PortQueueSharedBytes,
      kPortQueueNoAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
  return kPortQueue;
}

const PortQueue& getTHDefaultMCPortQueueSettings() {
  folly::MacAddress mac;
  static TomahawkAsic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::MULTICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, false /*is front panel port*/),
      bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      std::nullopt,
      kDefaultTHPortQueueSharedBytes,
      kPortQueueNoAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
  return kPortQueue;
}

const PortQueue& getTH3DefaultMCPortQueueSettings() {
  folly::MacAddress mac;
  static Tomahawk3Asic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::MULTICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, false /*is front panel port*/),
      bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      std::nullopt,
      kDefaultTH3PortQueueSharedBytes,
      kPortQueueNoAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
  return kPortQueue;
}

const PortQueue& getTH4DefaultMCPortQueueSettings() {
  folly::MacAddress mac;
  static Tomahawk4Asic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::MULTICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, false /*is front panel port*/),
      bcmAlphaToCfgAlpha(kDefaultPortQueueAlpha),
      std::nullopt,
      kDefaultTH4PortQueueSharedBytes,
      kPortQueueNoAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
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
    case cfg::StreamType::FABRIC_TX:
      break;
  }
  throw FbossError(
      "Port queue doesn't support streamType:",
      apache::thrift::util::enumNameSafe(streamType));
}

const PortQueue& getTD2DefaultMCCPUQueueSettings() {
  folly::MacAddress mac;
  static Trident2Asic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::MULTICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, true /*cpu port*/),
      std::nullopt,
      std::nullopt,
      kDefaultTD2PortQueueSharedBytes,
      kPortQueueNoAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
  return kPortQueue;
}

const PortQueue& getTHDefaultMCCPUQueueSettings() {
  folly::MacAddress mac;
  static TomahawkAsic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::MULTICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, true /*cpu port*/),
      std::nullopt,
      std::nullopt,
      kDefaultTHPortQueueSharedBytes,
      kPortQueueNoAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
  return kPortQueue;
}

const PortQueue& getTH3DefaultMCCPUQueueSettings() {
  folly::MacAddress mac;
  static Tomahawk3Asic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::MULTICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, true /*cpu port*/),
      std::nullopt,
      std::nullopt,
      kDefaultTH3PortQueueSharedBytes,
      kPortQueueNoAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
  return kPortQueue;
}

const PortQueue& getTH4DefaultMCCPUQueueSettings() {
  folly::MacAddress mac;
  static Tomahawk4Asic asic{
      cfg::SwitchType::NPU, std::nullopt, std::nullopt, mac};
  static const PortQueue kPortQueue{getPortQueueFields(
      kDefaultPortQueueId,
      kDefaultPortQueueScheduling,
      cfg::StreamType::MULTICAST,
      kDefaultPortQueueWeight,
      asic.getDefaultReservedBytes(
          cfg::StreamType::MULTICAST, true /*cpu port*/),
      std::nullopt,
      std::nullopt,
      kDefaultTH4PortQueueSharedBytes,
      kPortQueueNoAqm,
      getPortQueueRatePps(
          kDefaultPortQueuePacketsPerSecMin, kDefaultPortQueuePacketsPerSecMax),
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt)};
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
    case cfg::StreamType::FABRIC_TX:
      break;
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
