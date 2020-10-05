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

#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/LacpTypes.h"

namespace {
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

namespace facebook::fboss::utility {

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);
  return getQueueOutPacketsAndBytes(
      bcmSwitch->getPlatform()->useQueueGportForCos(),
      bcmSwitch->getUnit(),
      kCPUPort,
      queueId,
      true /* multicast queue*/);
}

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> defaultCpuAcls(
    const HwAsic* hwAsic) {
  std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> acls;

  auto createMatchAction = [](int queueId) {
    cfg::MatchAction action;
    cfg::QueueMatchAction queueAction;
    *queueAction.queueId_ref() = queueId;
    action.sendToQueue_ref() = queueAction;
    return action;
  };

  // multicast link local dst ip
  auto createNoOpAcl = [&](const folly::CIDRNetwork& dstNetwork) {
    cfg::AclEntry acl;
    auto dstIp =
        folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
    *acl.name_ref() =
        folly::to<std::string>("cpuPolicing-CPU-Port-Mcast-v6-", dstIp);

    acl.dstIp_ref() = dstIp;
    acl.srcPort_ref() = kCPUPort;

    acls.push_back(std::make_pair(acl, cfg::MatchAction{}));
  };
  createNoOpAcl(kIPv6LinkLocalMcastNetwork);

  // slow-protocols dst mac
  {
    cfg::AclEntry acl;
    *acl.name_ref() = "cpuPolicing-high-slow-protocols-mac";
    acl.dstMac_ref() = LACPDU::kSlowProtocolsDstMac().toString();
    acls.push_back(
        std::make_pair(acl, createMatchAction(getCoppHighPriQueueId(hwAsic))));
  }

  // dstClassL3 w/ BGP port to high pri queue
  // Preffered L4 ports. Combine these with local interfaces
  // to put locally destined traffic to these ports to hi-pri queue.
  auto createHighPriDstClassL3Acl = [&](bool isV4, bool isSrcPort) {
    cfg::AclEntry acl;
    *acl.name_ref() = folly::to<std::string>(
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
        std::make_pair(acl, createMatchAction(getCoppHighPriQueueId(hwAsic))));
  };
  createHighPriDstClassL3Acl(true, true);
  createHighPriDstClassL3Acl(true, false);
  createHighPriDstClassL3Acl(false, true);
  createHighPriDstClassL3Acl(false, false);

  // Dst IP local + DSCP 48 to high pri queue
  auto createHigPriLocalIpNetworkControlAcl = [&](bool isV4) {
    cfg::AclEntry acl;
    *acl.name_ref() = folly::to<std::string>(
        "cpuPolicing-high-",
        isV4 ? "dstLocalIp4" : "dstLocalIp6",
        "-network-control");
    acl.dscp_ref() = 48;
    acl.lookupClass_ref() = isV4 ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4
                                 : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6;
    acls.push_back(
        std::make_pair(acl, createMatchAction(getCoppHighPriQueueId(hwAsic))));
  };
  createHigPriLocalIpNetworkControlAcl(true);
  createHigPriLocalIpNetworkControlAcl(false);
  // Link local IPv6 + DSCP 48 to high pri queue
  auto createHighPriLinkLocalV6NetworoControlAcl =
      [&](const folly::CIDRNetwork& dstNetwork) {
        cfg::AclEntry acl;
        auto dstNetworkStr =
            folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
        *acl.name_ref() = folly::to<std::string>(
            "cpuPolicing-high-", dstNetworkStr, "-network-control");
        acl.dstIp_ref() = dstNetworkStr;
        acl.dscp_ref() = 48;
        acls.push_back(std::make_pair(
            acl, createMatchAction(getCoppHighPriQueueId(hwAsic))));
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
    *acl.name_ref() = folly::to<std::string>(
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
    *acl.name_ref() = folly::to<std::string>("cpuPolicing-mid-", dstIp);
    acl.dstIp_ref() = dstIp;

    acls.push_back(
        std::make_pair(acl, createMatchAction(utility::kCoppMidPriQueueId)));
  };
  createMidPriDstIpAcl(kIPv6LinkLocalMcastNetwork);

  // All fe80::/10 to mid pri queue
  createMidPriDstIpAcl(kIPv6LinkLocalUcastNetwork);

  return acls;
}

std::vector<cfg::PacketRxReasonToQueue> getCoppRxReasonToQueues(
    const HwAsic* hwAsic) {
  std::vector<cfg::PacketRxReasonToQueue> rxReasonToQueues;
  auto coppHighPriQueueId = utility::getCoppHighPriQueueId(hwAsic);
  std::vector<std::pair<cfg::PacketRxReason, uint16_t>>
      rxReasonToQueueMappings = {
          std::pair(cfg::PacketRxReason::ARP, coppHighPriQueueId),
          std::pair(cfg::PacketRxReason::DHCP, kCoppMidPriQueueId),
          std::pair(cfg::PacketRxReason::BPDU, kCoppMidPriQueueId),
          std::pair(cfg::PacketRxReason::L3_MTU_ERROR, kCoppLowPriQueueId),
          std::pair(cfg::PacketRxReason::L3_SLOW_PATH, kCoppLowPriQueueId),
          std::pair(cfg::PacketRxReason::L3_DEST_MISS, kCoppLowPriQueueId),
          std::pair(cfg::PacketRxReason::TTL_1, kCoppLowPriQueueId),
          std::pair(cfg::PacketRxReason::CPU_IS_NHOP, kCoppLowPriQueueId)};
  for (auto rxEntry : rxReasonToQueueMappings) {
    auto rxReasonToQueue = cfg::PacketRxReasonToQueue();
    rxReasonToQueue.rxReason_ref() = rxEntry.first;
    rxReasonToQueue.queueId_ref() = rxEntry.second;
    rxReasonToQueues.push_back(rxReasonToQueue);
  }
  return rxReasonToQueues;
}

} // namespace facebook::fboss::utility
