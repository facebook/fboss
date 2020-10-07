/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace {
constexpr uint32_t kCoppLowPriReservedBytes = 1040;
constexpr uint32_t kCoppDefaultPriReservedBytes = 1040;

constexpr uint32_t kCoppLowPriSharedBytes = 10192;
constexpr uint32_t kCoppDefaultPriSharedBytes = 10192;

} // unnamed namespace

namespace facebook::fboss::utility {

folly::CIDRNetwork kIPv6LinkLocalMcastNetwork() {
  return folly::IPAddress::createNetwork("ff02::/16");
}

folly::CIDRNetwork kIPv6LinkLocalUcastNetwork() {
  return folly::IPAddress::createNetwork("fe80::/10");
}

cfg::Range getRange(uint32_t minimum, uint32_t maximum) {
  cfg::Range range;
  range.minimum_ref() = minimum;
  range.maximum_ref() = maximum;

  return range;
}

uint16_t getCoppHighPriQueueId(const HwAsic* hwAsic) {
  switch (hwAsic->getAsicType()) {
    case HwAsic::AsicType::ASIC_TYPE_FAKE:
    case HwAsic::AsicType::ASIC_TYPE_MOCK:
    case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
      return 9;
    case HwAsic::AsicType::ASIC_TYPE_TAJO:
      return 7;
  }
  throw FbossError("Unexpected AsicType ", hwAsic->getAsicType());
}

cfg::StreamType getCpuDefaultStreamType(const HwAsic* hwAsic) {
  cfg::StreamType defaultStreamType = cfg::StreamType::MULTICAST;
  auto streamTypes = hwAsic->getQueueStreamTypes(true);
  if (streamTypes.begin() != streamTypes.end()) {
    defaultStreamType = *streamTypes.begin();
  }
  return defaultStreamType;
}

cfg::PortQueueRate setPortQueueRate(const HwAsic* hwAsic, uint16_t queueId) {
  uint32_t pps, bps;
  if (queueId == kCoppLowPriQueueId) {
    pps = kCoppLowPriPktsPerSec;
  } else if (queueId == kCoppDefaultPriQueueId) {
    pps = kCoppDefaultPriPktsPerSec;
  } else {
    throw FbossError("Unexpected queue id ", queueId);
  }

  auto portQueueRate = cfg::PortQueueRate();
  if (hwAsic->isSupported(HwAsic::Feature::SCHEDULER_PPS)) {
    portQueueRate.set_pktsPerSec(getRange(0, pps));
  } else {
    bps = pps * kAveragePacketSize * 8 / 1000;
    portQueueRate.set_kbitsPerSec(getRange(0, bps));
  }

  return portQueueRate;
}

void addCpuQueueConfig(cfg::SwitchConfig& config, const HwAsic* hwAsic) {
  std::vector<cfg::PortQueue> cpuQueues;

  cfg::PortQueue queue0;
  queue0.id = kCoppLowPriQueueId;
  queue0.name_ref() = "cpuQueue-low";
  queue0.streamType = getCpuDefaultStreamType(hwAsic);
  queue0.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight_ref() = kCoppLowPriWeight;
  queue0.portQueueRate_ref() = setPortQueueRate(hwAsic, kCoppLowPriQueueId);
  queue0.reservedBytes_ref() = kCoppLowPriReservedBytes;
  queue0.sharedBytes_ref() = kCoppLowPriSharedBytes;
  cpuQueues.push_back(queue0);

  cfg::PortQueue queue1;
  queue1.id = kCoppDefaultPriQueueId;
  queue1.name_ref() = "cpuQueue-default";
  queue1.streamType = getCpuDefaultStreamType(hwAsic);
  queue1.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue1.weight_ref() = kCoppDefaultPriWeight;
  queue1.portQueueRate_ref() = setPortQueueRate(hwAsic, kCoppDefaultPriQueueId);
  queue1.reservedBytes_ref() = kCoppDefaultPriReservedBytes;
  queue1.sharedBytes_ref() = kCoppDefaultPriSharedBytes;
  cpuQueues.push_back(queue1);

  cfg::PortQueue queue2;
  queue2.id = kCoppMidPriQueueId;
  queue2.name_ref() = "cpuQueue-mid";
  queue2.streamType = getCpuDefaultStreamType(hwAsic);
  queue2.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue2.weight_ref() = kCoppMidPriWeight;
  cpuQueues.push_back(queue2);

  cfg::PortQueue queue9;
  queue9.id = getCoppHighPriQueueId(hwAsic);
  queue9.name_ref() = "cpuQueue-high";
  queue9.streamType = getCpuDefaultStreamType(hwAsic);
  queue9.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue9.weight_ref() = kCoppHighPriWeight;
  cpuQueues.push_back(queue9);

  *config.cpuQueues_ref() = cpuQueues;
}

void setDefaultCpuTrafficPolicyConfig(
    cfg::SwitchConfig& config,
    const HwAsic* hwAsic) {
  auto cpuAcls = utility::defaultCpuAcls(hwAsic);
  // insert cpu acls into global acl field
  int curNumAcls = config.acls_ref()->size();
  config.acls_ref()->resize(curNumAcls + cpuAcls.size());
  for (int i = 0; i < cpuAcls.size(); i++) {
    config.acls_ref()[curNumAcls + i] = cpuAcls[i].first;
  }

  // prepare cpu traffic config
  cfg::CPUTrafficPolicyConfig cpuConfig;
  cfg::TrafficPolicyConfig trafficConfig;
  trafficConfig.matchToAction_ref()->resize(cpuAcls.size());
  for (int i = 0; i < cpuAcls.size(); i++) {
    *trafficConfig.matchToAction_ref()[i].matcher_ref() =
        *cpuAcls[i].first.name_ref();
    *trafficConfig.matchToAction_ref()[i].action_ref() = cpuAcls[i].second;
  }
  cpuConfig.trafficPolicy_ref() = trafficConfig;
  auto rxReasonToQueues = getCoppRxReasonToQueues(hwAsic);
  if (rxReasonToQueues.size()) {
    cpuConfig.rxReasonToQueueOrderedList_ref() = rxReasonToQueues;
  }
  config.cpuTrafficPolicy_ref() = cpuConfig;
}

cfg::MatchAction createQueueMatchAction(int queueId) {
  cfg::MatchAction action;
  cfg::QueueMatchAction queueAction;
  queueAction.queueId_ref() = queueId;
  action.sendToQueue_ref() = queueAction;
  return action;
}

void addNoActionAclForNw(
    const folly::CIDRNetwork& nw,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls) {
  cfg::AclEntry acl;
  auto dstIp = folly::to<std::string>(nw.first, "/", nw.second);
  acl.name_ref() =
      folly::to<std::string>("cpuPolicing-CPU-Port-Mcast-v6-", dstIp);

  acl.dstIp_ref() = dstIp;
  acl.srcPort_ref() = kCPUPort;
  acls.push_back(std::make_pair(acl, cfg::MatchAction{}));
}

void addHighPriAclForNwAndNetworoControlDscp(
    const folly::CIDRNetwork& dstNetwork,
    int highPriQueueId,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls) {
  cfg::AclEntry acl;
  auto dstNetworkStr =
      folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
  acl.name_ref() = folly::to<std::string>(
      "cpuPolicing-high-", dstNetworkStr, "-network-control");
  acl.dstIp_ref() = dstNetworkStr;
  acl.dscp_ref() = 48;
  acls.push_back(std::make_pair(acl, createQueueMatchAction(highPriQueueId)));
}

void addMidPriAclForNw(
    const folly::CIDRNetwork& dstNetwork,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls) {
  cfg::AclEntry acl;
  auto dstIp = folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
  acl.name_ref() = folly::to<std::string>("cpuPolicing-mid-", dstIp);
  acl.dstIp_ref() = dstIp;

  acls.push_back(
      std::make_pair(acl, createQueueMatchAction(utility::kCoppMidPriQueueId)));
}

} // namespace facebook::fboss::utility
