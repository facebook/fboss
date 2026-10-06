/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/config/agent/CoppConfigUtils.h"

#include <string_view>
#include <utility>
#include <vector>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/lib/config/agent/AclConfigUtils.h"

namespace facebook::fboss::utility {
namespace {

constexpr uint16_t kLowPriorityQueueId = 0;
constexpr uint16_t kDefaultPriorityQueueId = 1;
constexpr uint32_t kLowPriorityWeight = 1;
constexpr uint32_t kDefaultPriorityWeight = 1;
constexpr uint32_t kMidPriorityWeight = 2;
constexpr uint32_t kHighPriorityWeight = 4;

constexpr uint32_t kAveragePacketSize = 300;
constexpr uint32_t kCpuPacketOverheadBytes = 52;
constexpr uint32_t kLowPriorityPacketsPerSecond = 100;
constexpr uint32_t kDefaultPriorityPacketsPerSecond = 200;
constexpr uint32_t kTajoLowPriorityPacketsPerSecond = 10000;
constexpr uint32_t kTajoDefaultPriorityPacketsPerSecond = 20000;
constexpr uint32_t kDnxLowPriorityPacketsPerSecond = 12000;
constexpr uint32_t kDnxDefaultPriorityPacketsPerSecond = 24000;
constexpr uint32_t kDnxLowPriorityKbitsPerSecond = 100 * 1024;

constexpr uint8_t kNetworkControlDscp = 48;
constexpr std::string_view kIPv6LinkLocalNetwork = "fe80::/10";
constexpr std::string_view kIPv6LinkLocalMulticastNetwork = "ff02::/16";

void setWeight(cfg::PortQueue& queue, uint32_t weight) {
  if (*queue.scheduling() != cfg::QueueScheduling::STRICT_PRIORITY &&
      *queue.scheduling() != cfg::QueueScheduling::INTERNAL) {
    queue.weight() = weight;
  }
}

cfg::PortQueue makeCpuQueue(
    const HwAsic& asic,
    uint16_t queueId,
    std::string_view name,
    uint32_t weight,
    bool rateLimited) {
  cfg::PortQueue queue;
  queue.id() = queueId;
  queue.name() = name;
  queue.streamType() = getCpuDefaultStreamType(&asic);
  queue.scheduling() = getCpuDefaultQueueScheduling(&asic);
  setWeight(queue, weight);
  if (rateLimited) {
    queue.portQueueRate() = getPortQueueRate(&asic, queueId);
  }
  return queue;
}

cfg::PacketRxReasonToQueue makeRxReasonToQueue(
    cfg::PacketRxReason reason,
    uint16_t queueId) {
  cfg::PacketRxReasonToQueue entry;
  entry.rxReason() = reason;
  entry.queueId() = queueId;
  return entry;
}

cfg::MatchAction makeQueueAction(const HwAsic& asic, uint16_t queueId) {
  cfg::QueueMatchAction queueAction;
  queueAction.queueId() = queueId;
  cfg::MatchAction action;
  action.sendToQueue() = queueAction;
  action.toCpuAction() = getCpuActionType(&asic);
  return action;
}

std::pair<cfg::AclEntry, cfg::MatchAction> makeNetworkAcl(
    const HwAsic& asic,
    std::string_view name,
    std::string_view network,
    uint16_t queueId,
    bool networkControl) {
  cfg::AclEntry acl;
  acl.name() = name;
  acl.dstIp() = network;
  if (networkControl) {
    acl.dscp() = kNetworkControlDscp;
  }
  return {std::move(acl), makeQueueAction(asic, queueId)};
}

} // namespace

cfg::Range getRange(uint32_t minimum, uint32_t maximum) {
  cfg::Range range;
  range.minimum() = minimum;
  range.maximum() = maximum;
  return range;
}

cfg::StreamType getCpuDefaultStreamType(const HwAsic* hwAsic) {
  auto streamTypes = hwAsic->getQueueStreamTypes(cfg::PortType::CPU_PORT);
  return streamTypes.empty() ? cfg::StreamType::MULTICAST
                             : *streamTypes.begin();
}

cfg::QueueScheduling getCpuDefaultQueueScheduling(const HwAsic* hwAsic) {
  if (hwAsic->getAsicVendor() == HwAsic::AsicVendor::ASIC_VENDOR_CHENAB) {
    return cfg::QueueScheduling::STRICT_PRIORITY;
  }
  return cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
}

uint32_t getCoppQueuePps(const HwAsic* hwAsic, uint16_t queueId) {
  if (hwAsic->getAsicVendor() == HwAsic::AsicVendor::ASIC_VENDOR_TAJO) {
    if (queueId == kLowPriorityQueueId) {
      return kTajoLowPriorityPacketsPerSecond;
    }
    if (queueId == kDefaultPriorityQueueId) {
      return kTajoDefaultPriorityPacketsPerSecond;
    }
  } else if (hwAsic->getSwitchType() == cfg::SwitchType::VOQ) {
    if (queueId == kLowPriorityQueueId) {
      return kDnxLowPriorityPacketsPerSecond;
    }
    if (queueId == kDefaultPriorityQueueId) {
      return kDnxDefaultPriorityPacketsPerSecond;
    }
  } else {
    if (queueId == kLowPriorityQueueId) {
      return kLowPriorityPacketsPerSecond;
    }
    if (queueId == kDefaultPriorityQueueId) {
      return kDefaultPriorityPacketsPerSecond;
    }
  }
  throw FbossError("Unexpected CoPP queue id ", queueId);
}

cfg::PortQueueRate getPortQueueRate(const HwAsic* hwAsic, uint16_t queueId) {
  const auto pps = getCoppQueuePps(hwAsic, queueId);
  cfg::PortQueueRate rate;
  if (hwAsic->isSupported(HwAsic::Feature::SCHEDULER_PPS)) {
    rate.pktsPerSec() = getRange(0, pps);
    return rate;
  }

  uint32_t kbps;
  if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3 ||
      hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO4 ||
      hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_QUMRAN4D) {
    kbps = kDnxLowPriorityKbitsPerSecond;
  } else if (
      hwAsic->getAsicVendor() == HwAsic::AsicVendor::ASIC_VENDOR_TAJO ||
      hwAsic->getAsicVendor() == HwAsic::AsicVendor::ASIC_VENDOR_CHENAB ||
      hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWKULTRA1) {
    kbps = (pps / 60 * 60) * (kAveragePacketSize + kCpuPacketOverheadBytes) *
        8 / 1000;
  } else {
    throw FbossError("CoPP queue rate unsupported for ASIC");
  }
  rate.kbitsPerSec() = getRange(0, kbps);
  return rate;
}

cfg::ToCpuAction getCpuActionType(const HwAsic* hwAsic) {
  switch (hwAsic->getAsicType()) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_FAKE_NO_WARMBOOT:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK6:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWKULTRA1:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_P200:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_G202X:
      return cfg::ToCpuAction::COPY;
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
    case cfg::AsicType::ASIC_TYPE_JERICHO4:
    case cfg::AsicType::ASIC_TYPE_QUMRAN4D:
    case cfg::AsicType::ASIC_TYPE_CHENAB:
    case cfg::AsicType::ASIC_TYPE_CHENAB2:
      return cfg::ToCpuAction::TRAP;
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_AGERA3:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      break;
  }
  throw FbossError(
      "ASIC does not support a CPU action: ", hwAsic->getAsicType());
}

void addDefaultCpuQueueConfig(cfg::SwitchConfig& config, const HwAsic& asic) {
  if (asic.getSwitchType() != cfg::SwitchType::NPU ||
      !asic.isSupported(HwAsic::Feature::CPU_QUEUES)) {
    return;
  }
  config.cpuQueues() = {
      makeCpuQueue(
          asic,
          asic.getHiPriCpuQueueId(),
          "cpuQueue-high",
          kHighPriorityWeight,
          false),
      makeCpuQueue(
          asic,
          asic.getMidPriCpuQueueId(),
          "cpuQueue-mid",
          kMidPriorityWeight,
          false),
      makeCpuQueue(
          asic,
          kDefaultPriorityQueueId,
          "cpuQueue-default",
          kDefaultPriorityWeight,
          true),
      makeCpuQueue(
          asic, kLowPriorityQueueId, "cpuQueue-low", kLowPriorityWeight, true),
  };
}

void addDefaultCpuTrafficPolicyConfig(
    cfg::SwitchConfig& config,
    const HwAsic& asic) {
  if (asic.getSwitchType() != cfg::SwitchType::NPU ||
      !asic.isSupported(HwAsic::Feature::CPU_QUEUES)) {
    return;
  }

  const auto highQueueId = asic.getHiPriCpuQueueId();
  const auto midQueueId = asic.getMidPriCpuQueueId();
  std::vector<cfg::PacketRxReasonToQueue> rxReasons;
  if (asic.isSupported(HwAsic::Feature::NO_RX_REASON_TRAP)) {
    rxReasons = {
        makeRxReasonToQueue(cfg::PacketRxReason::TTL_1, kLowPriorityQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::DHCP, midQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::DHCPV6, midQueueId),
    };
  } else {
    rxReasons = {
        makeRxReasonToQueue(cfg::PacketRxReason::NDP, highQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::ARP, highQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::ARP_RESPONSE, highQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::BGP, highQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::BGPV6, highQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::LACP, highQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::LLDP, midQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::CPU_IS_NHOP, midQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::DHCP, midQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::DHCPV6, midQueueId),
        makeRxReasonToQueue(cfg::PacketRxReason::TTL_1, kLowPriorityQueueId),
    };
  }
  if (asic.isSupported(HwAsic::Feature::L3_MTU_ERROR_TRAP)) {
    rxReasons.push_back(makeRxReasonToQueue(
        cfg::PacketRxReason::L3_MTU_ERROR, kLowPriorityQueueId));
  } else if (asic.isSupported(HwAsic::Feature::PORT_MTU_ERROR_TRAP)) {
    rxReasons.push_back(makeRxReasonToQueue(
        cfg::PacketRxReason::PORT_MTU_ERROR, kLowPriorityQueueId));
  }
  if (!asic.isSupported(HwAsic::Feature::NO_RX_REASON_TRAP)) {
    // UNMATCHED is the fallback reason and must remain last in this ordered
    // list so it does not take priority over a more specific trap reason.
    rxReasons.push_back(makeRxReasonToQueue(
        cfg::PacketRxReason::UNMATCHED, kDefaultPriorityQueueId));
  }

  std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> cpuAcls;
  cfg::AclEntry cpuMulticastAcl;
  cpuMulticastAcl.name() = "cpuPolicing-CPU-Port-Mcast-v6";
  cpuMulticastAcl.dstIp() = kIPv6LinkLocalMulticastNetwork;
  cpuMulticastAcl.srcPort() =
      cfg::switch_config_constants::CPU_PORT_LOGICALID();
  cpuAcls.emplace_back(std::move(cpuMulticastAcl), cfg::MatchAction{});
  cpuAcls.push_back(makeNetworkAcl(
      asic,
      "cpuPolicing-high-NetworkControl-linkLocal-v6",
      kIPv6LinkLocalNetwork,
      highQueueId,
      true));
  cpuAcls.push_back(makeNetworkAcl(
      asic,
      "cpuPolicing-high-NetworkControl-ff02::/16",
      kIPv6LinkLocalMulticastNetwork,
      highQueueId,
      true));
  cpuAcls.push_back(makeNetworkAcl(
      asic,
      "cpuPolicing-mid-linkLocal-v6",
      kIPv6LinkLocalNetwork,
      midQueueId,
      false));
  cpuAcls.push_back(makeNetworkAcl(
      asic,
      "cpuPolicing-mid-ff02::/16",
      kIPv6LinkLocalMulticastNetwork,
      midQueueId,
      false));

  cfg::TrafficPolicyConfig trafficPolicy;
  for (auto& [acl, action] : cpuAcls) {
    cfg::MatchToAction matchToAction;
    matchToAction.matcher() = *acl.name();
    matchToAction.action() = std::move(action);
    trafficPolicy.matchToAction()->push_back(std::move(matchToAction));
    addAclEntryToDefaultAclTable(config, std::move(acl));
  }
  cfg::CPUTrafficPolicyConfig cpuPolicy;
  cpuPolicy.trafficPolicy() = std::move(trafficPolicy);
  cpuPolicy.rxReasonToQueueOrderedList() = std::move(rxReasons);
  config.cpuTrafficPolicy() = std::move(cpuPolicy);
}

} // namespace facebook::fboss::utility
