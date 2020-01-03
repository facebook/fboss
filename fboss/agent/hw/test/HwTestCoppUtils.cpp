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

#include "fboss/agent/LacpTypes.h"

namespace {
constexpr uint32_t kCoppLowPriReservedBytes = 1040;
constexpr uint32_t kCoppDefaultPriReservedBytes = 1040;

constexpr uint32_t kCoppLowPriSharedBytes = 10192;
constexpr uint32_t kCoppDefaultPriSharedBytes = 10192;

/**
 * Link-local multicast network
 */
const auto kIPv6LinkLocalMcastNetwork =
    folly::IPAddress::createNetwork("ff02::/16");
/*
 * Link local ucast network
 */
const auto kIPv6LinkLocalUcastNetwork =
    folly::IPAddress::createNetwork("fe80::/10");
} // unnamed namespace

namespace facebook::fboss {

namespace utility {

cfg::Range getRange(uint32_t minimum, uint32_t maximum) {
  cfg::Range range;
  range.set_minimum(minimum);
  range.set_maximum(maximum);

  return range;
}

void addCpuQueueConfig(cfg::SwitchConfig& config) {
  std::vector<cfg::PortQueue> cpuQueues;

  cfg::PortQueue queue0;
  queue0.id = kCoppLowPriQueueId;
  queue0.name_ref().value_unchecked() = "cpuQueue-low";
  queue0.streamType = cfg::StreamType::MULTICAST;
  queue0.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight_ref() = kCoppLowPriWeight;
  queue0.portQueueRate_ref().value_unchecked().set_pktsPerSec(
      getRange(0, kCoppLowPriPktsPerSec));
  queue0.__isset.portQueueRate = true;
  queue0.reservedBytes_ref() = kCoppLowPriReservedBytes;
  queue0.sharedBytes_ref() = kCoppLowPriSharedBytes;
  cpuQueues.push_back(queue0);

  cfg::PortQueue queue1;
  queue1.id = kCoppDefaultPriQueueId;
  queue1.name_ref().value_unchecked() = "cpuQueue-default";
  queue1.streamType = cfg::StreamType::MULTICAST;
  queue1.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue1.weight_ref() = kCoppDefaultPriWeight;
  queue1.portQueueRate_ref().value_unchecked().set_pktsPerSec(
      getRange(0, kCoppDefaultPriPktsPerSec));
  queue1.__isset.portQueueRate = true;
  queue1.reservedBytes_ref() = kCoppDefaultPriReservedBytes;
  queue1.sharedBytes_ref() = kCoppDefaultPriSharedBytes;
  cpuQueues.push_back(queue1);

  cfg::PortQueue queue2;
  queue2.id = kCoppMidPriQueueId;
  queue2.name_ref().value_unchecked() = "cpuQueue-mid";
  queue2.streamType = cfg::StreamType::MULTICAST;
  queue2.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue2.weight_ref() = kCoppMidPriWeight;
  cpuQueues.push_back(queue2);

  cfg::PortQueue queue9;
  queue9.id = kCoppHighPriQueueId;
  queue9.name_ref().value_unchecked() = "cpuQueue-high";
  queue9.streamType = cfg::StreamType::MULTICAST;
  queue9.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue9.weight_ref() = kCoppHighPriWeight;
  cpuQueues.push_back(queue9);

  config.cpuQueues = cpuQueues;
}

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> defaultCpuAcls(
    const folly::MacAddress& localMac) {
  std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> acls;

  auto createMatchAction = [](int queueId) {
    cfg::MatchAction action;
    cfg::QueueMatchAction queueAction;
    queueAction.queueId = queueId;
    action.sendToQueue_ref() = queueAction;
    return action;
  };

  // slow-protocols dst mac
  {
    cfg::AclEntry acl;
    acl.name = "cpuPolicing-high-slow-protocols-mac";
    acl.dstMac_ref() = LACPDU::kSlowProtocolsDstMac().toString();
    acls.push_back(
        std::make_pair(acl, createMatchAction(utility::kCoppHighPriQueueId)));
  }

  // dstClassL3 w/ BGP port to high pri queue
  // Preffered L4 ports. Combine these with local interfaces
  // to put locally destined traffic to these ports to hi-pri queue.
  auto createHighPriDstClassL3Acl = [&](bool isV4, bool isSrcPort) {
    cfg::AclEntry acl;
    acl.name = folly::to<std::string>(
        "cpuPolicing-high-",
        isV4 ? "dstLocalIp4-" : "dstLocalIp6-",
        isSrcPort ? "srcPort:" : "dstPrt:",
        utility::kBgpPort);
    acl.lookupClass_ref() = isV4 ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4
                                 : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6;
    if (isSrcPort) {
      acl.l4SrcPort_ref() = utility::kBgpPort;
    } else {
      acl.l4DstPort_ref() = utility::kBgpPort;
    }

    acls.push_back(
        std::make_pair(acl, createMatchAction(utility::kCoppHighPriQueueId)));
  };
  createHighPriDstClassL3Acl(true, true);
  createHighPriDstClassL3Acl(true, false);
  createHighPriDstClassL3Acl(false, true);
  createHighPriDstClassL3Acl(false, false);

  // Dst IP local + DSCP 48 to high pri queue
  auto createHigPriLocalIpNetworkControlAcl = [&](bool isV4) {
    cfg::AclEntry acl;
    acl.name = folly::to<std::string>(
        "cpuPolicing-high-",
        isV4 ? "dstLocalIp4" : "dstLocalIp6",
        "-network-control");
    acl.dscp_ref() = 48;
    acl.lookupClass_ref() = isV4 ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4
                                 : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6;
    acls.push_back(
        std::make_pair(acl, createMatchAction(utility::kCoppHighPriQueueId)));
  };
  createHigPriLocalIpNetworkControlAcl(true);
  createHigPriLocalIpNetworkControlAcl(false);
  // Link local IPv6 + DSCP 48 to high pri queue
  auto createHighPriLinkLocalV6NetworoControlAcl =
      [&](const folly::CIDRNetwork& dstNetwork) {
        cfg::AclEntry acl;
        auto dstNetworkStr =
            folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
        acl.name = folly::to<std::string>(
            "cpuPolicing-high-", dstNetworkStr, "-network-control");
        acl.dstIp_ref() = dstNetworkStr;
        acl.dscp_ref() = 48;
        acls.push_back(std::make_pair(
            acl, createMatchAction(utility::kCoppHighPriQueueId)));
      };
  createHighPriLinkLocalV6NetworoControlAcl(kIPv6LinkLocalMcastNetwork);
  createHighPriLinkLocalV6NetworoControlAcl(kIPv6LinkLocalUcastNetwork);

  // Now steer traffic destined to this (local) interface IP
  // to mid pri queue. Note that we add this Acl entry *after*
  // (with a higher Acl ID) than locally destined protocol
  // traffic. Acl entries are matched in order, so we need to
  // go from more specific to less specific matches.
  auto createMidPriDstClassL3Acl = [&](bool isV4) {
    cfg::AclEntry acl;
    acl.name = folly::to<std::string>(
        "cpuPolicing-mid-", isV4 ? "dstLocalIp4" : "dstLocalIp6");
    acl.lookupClass_ref() = isV4 ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4
                                 : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6;

    acls.push_back(
        std::make_pair(acl, createMatchAction(utility::kCoppMidPriQueueId)));
  };
  createMidPriDstClassL3Acl(true);
  createMidPriDstClassL3Acl(false);

  // unicast and multicast link local dst ip
  auto createMidPriDstIpAcl = [&](const folly::CIDRNetwork& dstNetwork) {
    cfg::AclEntry acl;
    auto dstIp =
        folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
    acl.name = folly::to<std::string>("cpuPolicing-mid-", dstIp);
    acl.dstIp_ref() = dstIp;

    acls.push_back(
        std::make_pair(acl, createMatchAction(utility::kCoppMidPriQueueId)));
  };
  createMidPriDstIpAcl(kIPv6LinkLocalMcastNetwork);

  // All fe80::/10 to mid pri queue
  createMidPriDstIpAcl(kIPv6LinkLocalUcastNetwork);

  return acls;
}

void setDefaultCpuTrafficPolicyConfig(
    cfg::SwitchConfig& config,
    const folly::MacAddress& localMac) {
  auto cpuAcls = utility::defaultCpuAcls(localMac);
  // insert cpu acls into global acl field
  int curNumAcls = config.acls.size();
  config.acls.resize(curNumAcls + cpuAcls.size());
  for (int i = 0; i < cpuAcls.size(); i++) {
    config.acls[curNumAcls + i] = cpuAcls[i].first;
  }

  // prepare cpu traffic config
  cfg::CPUTrafficPolicyConfig cpuConfig;
  cfg::TrafficPolicyConfig trafficConfig;
  trafficConfig.matchToAction.resize(cpuAcls.size());
  for (int i = 0; i < cpuAcls.size(); i++) {
    trafficConfig.matchToAction[i].matcher = cpuAcls[i].first.name;
    trafficConfig.matchToAction[i].action = cpuAcls[i].second;
  }
  cpuConfig.trafficPolicy_ref() = trafficConfig;
  auto rxReasonToQueues = getCoppRxReasonToQueues();
  if (rxReasonToQueues.size()) {
    cpuConfig.set_rxReasonToCPUQueue(rxReasonToQueues);
  }
  config.cpuTrafficPolicy_ref() = cpuConfig;
}
} // namespace utility

} // namespace facebook::fboss
