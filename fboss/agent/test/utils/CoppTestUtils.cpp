/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <vector>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"

DECLARE_bool(enable_acl_table_group);

namespace {
constexpr int kDownlinkBaseVlanId = 2000;
constexpr uint32_t kCoppLowPriReservedBytes = 1040;
constexpr uint32_t kCoppDefaultPriReservedBytes = 1040;
constexpr uint32_t kBcmCoppLowPriSharedBytes = 10192;
constexpr uint32_t kBcmCoppDefaultPriSharedBytes = 10192;
const std::string kMplsDestNoMatchAclName = "cpuPolicing-mpls-dest-nomatch";
const std::string kMplsDestNoMatchCounterName = "mpls-dest-nomatch-counter";
} // unnamed namespace

namespace facebook::fboss::utility {

std::string getMplsDestNoMatchCounterName() {
  return kMplsDestNoMatchCounterName;
}

folly::CIDRNetwork kIPv6LinkLocalMcastNetwork() {
  return folly::IPAddress::createNetwork("ff02::/16");
}

folly::CIDRNetwork kIPv6LinkLocalUcastNetwork() {
  return folly::IPAddress::createNetwork("fe80::/10");
}

folly::CIDRNetwork kIPv6NdpSolicitNetwork() {
  return folly::IPAddress::createNetwork("ff02:0:0:0:0:1:ff00::/104");
}

cfg::Range getRange(uint32_t minimum, uint32_t maximum) {
  cfg::Range range;
  range.minimum() = minimum;
  range.maximum() = maximum;

  return range;
}

uint16_t getCoppHighPriQueueId(const HwAsic* hwAsic) {
  switch (hwAsic->getAsicType()) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      return 9;
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
      return 7;
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      throw FbossError(
          "AsicType ", hwAsic->getAsicType(), " doesn't support queue feature");
  }
  throw FbossError("Unexpected AsicType ", hwAsic->getAsicType());
}

cfg::ToCpuAction getCpuActionType(const HwAsic* hwAsic) {
  switch (hwAsic->getAsicType()) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_YUBA:
      return cfg::ToCpuAction::COPY;
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
      return cfg::ToCpuAction::TRAP;
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      throw FbossError(
          "AsicType ", hwAsic->getAsicType(), " doesn't support cpu action");
  }
  throw FbossError("Unexpected AsicType ", hwAsic->getAsicType());
}

cfg::StreamType getCpuDefaultStreamType(const HwAsic* hwAsic) {
  cfg::StreamType defaultStreamType = cfg::StreamType::MULTICAST;
  auto streamTypes = hwAsic->getQueueStreamTypes(cfg::PortType::CPU_PORT);
  if (streamTypes.begin() != streamTypes.end()) {
    defaultStreamType = *streamTypes.begin();
  }
  return defaultStreamType;
}

uint32_t getCoppQueuePps(const HwAsic* hwAsic, uint16_t queueId) {
  uint32_t pps;
  if (hwAsic->getAsicVendor() == HwAsic::AsicVendor::ASIC_VENDOR_TAJO) {
    if (queueId == kCoppLowPriQueueId) {
      pps = kCoppTajoLowPriPktsPerSec;
    } else if (queueId == kCoppDefaultPriQueueId) {
      pps = kCoppTajoDefaultPriPktsPerSec;
    } else {
      throw FbossError("Unexpected queue id ", queueId);
    }
  } else if (hwAsic->getSwitchType() == cfg::SwitchType::VOQ) {
    if (queueId == kCoppLowPriQueueId) {
      pps = kCoppDnxLowPriPktsPerSec;
    } else if (queueId == kCoppDefaultPriQueueId) {
      pps = kCoppDnxDefaultPriPktsPerSec;
    } else {
      throw FbossError("Unexpected queue id ", queueId);
    }
  } else {
    if (queueId == kCoppLowPriQueueId) {
      pps = kCoppLowPriPktsPerSec;
    } else if (queueId == kCoppDefaultPriQueueId) {
      pps = kCoppDefaultPriPktsPerSec;
    } else {
      throw FbossError("Unexpected queue id ", queueId);
    }
  }
  return pps;
}

uint32_t getCoppQueueKbpsFromPps(const HwAsic* hwAsic, uint32_t pps) {
  uint32_t kbps;
  if (hwAsic->getAsicVendor() == HwAsic::AsicVendor::ASIC_VENDOR_TAJO) {
    kbps = (round(pps / 60) * 60) *
        (kAveragePacketSize + kCpuPacketOverheadBytes) * 8 / 1000;
  } else {
    throw FbossError("Copp queue pps to kbps unsupported for platform");
  }
  return kbps;
}

cfg::PortQueueRate setPortQueueRate(const HwAsic* hwAsic, uint16_t queueId) {
  uint32_t pps = getCoppQueuePps(hwAsic, queueId);
  auto portQueueRate = cfg::PortQueueRate();

  if (hwAsic->isSupported(HwAsic::Feature::SCHEDULER_PPS)) {
    portQueueRate.pktsPerSec_ref() = getRange(0, pps);
  } else {
    uint32_t kbps = getCoppQueueKbpsFromPps(hwAsic, pps);
    portQueueRate.kbitsPerSec_ref() = getRange(0, kbps);
  }

  return portQueueRate;
}

void addCpuQueueConfig(
    cfg::SwitchConfig& config,
    const HwAsic* hwAsic,
    bool isSai,
    bool setQueueRate) {
  std::vector<cfg::PortQueue> cpuQueues;

  cfg::PortQueue queue0;
  queue0.id() = kCoppLowPriQueueId;
  queue0.name() = "cpuQueue-low";
  queue0.streamType() = getCpuDefaultStreamType(hwAsic);
  queue0.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight() = kCoppLowPriWeight;
  if (setQueueRate) {
    queue0.portQueueRate() = setPortQueueRate(hwAsic, kCoppLowPriQueueId);
  }
  if (!hwAsic->mmuQgroupsEnabled()) {
    queue0.reservedBytes() = kCoppLowPriReservedBytes;
  }
  setPortQueueSharedBytes(queue0, isSai);
  cpuQueues.push_back(queue0);

  cfg::PortQueue queue1;
  queue1.id() = kCoppDefaultPriQueueId;
  queue1.name() = "cpuQueue-default";
  queue1.streamType() = getCpuDefaultStreamType(hwAsic);
  queue1.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue1.weight() = kCoppDefaultPriWeight;
  if (setQueueRate) {
    queue1.portQueueRate() = setPortQueueRate(hwAsic, kCoppDefaultPriQueueId);
  }
  if (!hwAsic->mmuQgroupsEnabled()) {
    queue1.reservedBytes() = kCoppDefaultPriReservedBytes;
  }
  setPortQueueSharedBytes(queue1, isSai);
  cpuQueues.push_back(queue1);

  cfg::PortQueue queue2;
  queue2.id() = kCoppMidPriQueueId;
  queue2.name() = "cpuQueue-mid";
  queue2.streamType() = getCpuDefaultStreamType(hwAsic);
  queue2.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue2.weight() = kCoppMidPriWeight;
  cpuQueues.push_back(queue2);

  cfg::PortQueue queue9;
  queue9.id() = getCoppHighPriQueueId(hwAsic);
  queue9.name() = "cpuQueue-high";
  queue9.streamType() = getCpuDefaultStreamType(hwAsic);
  queue9.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue9.weight() = kCoppHighPriWeight;
  cpuQueues.push_back(queue9);

  *config.cpuQueues() = cpuQueues;
}

void setDefaultCpuTrafficPolicyConfig(
    cfg::SwitchConfig& config,
    const HwAsic* hwAsic,
    bool isSai) {
  auto cpuAcls = utility::defaultCpuAcls(hwAsic, config, isSai);

  for (int i = 0; i < cpuAcls.size(); i++) {
    utility::addAclEntry(&config, cpuAcls[i].first, std::nullopt);
  }

  // prepare cpu traffic config
  cfg::CPUTrafficPolicyConfig cpuConfig;
  cfg::TrafficPolicyConfig trafficConfig;
  trafficConfig.matchToAction()->resize(cpuAcls.size());
  for (int i = 0; i < cpuAcls.size(); i++) {
    *trafficConfig.matchToAction()[i].matcher() = *cpuAcls[i].first.name();
    *trafficConfig.matchToAction()[i].action() = cpuAcls[i].second;
  }

  if (config.cpuTrafficPolicy() && config.cpuTrafficPolicy()->trafficPolicy() &&
      config.cpuTrafficPolicy()->trafficPolicy()->defaultQosPolicy()) {
    trafficConfig.defaultQosPolicy() =
        *config.cpuTrafficPolicy()->trafficPolicy()->defaultQosPolicy();
  }

  cpuConfig.trafficPolicy() = trafficConfig;
  auto rxReasonToQueues = getCoppRxReasonToQueues(hwAsic, isSai);
  if (rxReasonToQueues.size()) {
    cpuConfig.rxReasonToQueueOrderedList() = rxReasonToQueues;
  }
  config.cpuTrafficPolicy() = cpuConfig;
}

uint16_t getNumDefaultCpuAcls(const HwAsic* hwAsic, bool isSai) {
  cfg::SwitchConfig config; // unused
  return utility::defaultCpuAcls(hwAsic, config, isSai).size();
}

cfg::MatchAction
createQueueMatchAction(int queueId, bool isSai, cfg::ToCpuAction toCpuAction) {
  if (toCpuAction != cfg::ToCpuAction::COPY &&
      toCpuAction != cfg::ToCpuAction::TRAP) {
    throw FbossError("Unsupported CounterType for ACL");
  }
  return utility::getToQueueAction(queueId, isSai, toCpuAction);
}

void addNoActionAclForNw(
    const folly::CIDRNetwork& nw,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls) {
  cfg::AclEntry acl;
  auto dstIp = folly::to<std::string>(nw.first, "/", nw.second);
  acl.name() = folly::to<std::string>("cpuPolicing-CPU-Port-Mcast-v6-", dstIp);

  acl.dstIp() = dstIp;
  acl.srcPort() = kCPUPort;
  acls.push_back(std::make_pair(acl, cfg::MatchAction{}));
}

void addHighPriAclForNwAndNetworkControlDscp(
    const folly::CIDRNetwork& dstNetwork,
    int highPriQueueId,
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai) {
  cfg::AclEntry acl;
  auto dstNetworkStr =
      folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
  acl.name() = folly::to<std::string>(
      "cpuPolicing-high-", dstNetworkStr, "-network-control");
  acl.dstIp() = dstNetworkStr;
  acl.dscp() = 48;
  acls.push_back(std::make_pair(
      acl, createQueueMatchAction(highPriQueueId, isSai, toCpuAction)));
}

void addMidPriAclForNw(
    const folly::CIDRNetwork& dstNetwork,
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai) {
  cfg::AclEntry acl;
  auto dstIp = folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
  acl.name() = folly::to<std::string>("cpuPolicing-mid-", dstIp);
  acl.dstIp() = dstIp;

  acls.push_back(std::make_pair(
      acl,
      createQueueMatchAction(utility::kCoppMidPriQueueId, isSai, toCpuAction)));
}

void addLowPriAclForUnresolvedRoutes(
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai) {
  cfg::AclEntry acl;
  acl.name() = folly::to<std::string>("cpu-unresolved-route-acl");
  acl.lookupClassRoute() = cfg::AclLookupClass::CLASS_UNRESOLVED_ROUTE_TO_CPU;
  acls.push_back(std::make_pair(
      acl,
      createQueueMatchAction(utility::kCoppLowPriQueueId, isSai, toCpuAction)));
}

/*
 * This API is invoked once HwPortStats for all queues is collected
 * with getCpuQueueWatermarkStats() which does a clear-on-read. The
 * purpose is to return per queue WatermarkBytes from HwPortStats.
 */
uint64_t getCpuQueueWatermarkBytes(HwPortStats& hwPortStats, int queueId) {
  auto queueIter = hwPortStats.queueWatermarkBytes_()->find(queueId);
  return (
      (queueIter != hwPortStats.queueWatermarkBytes_()->end())
          ? queueIter->second
          : 0);
}

std::shared_ptr<facebook::fboss::Interface> getEligibleInterface(
    std::shared_ptr<SwitchState> swState) {
  VlanID downlinkBaseVlanId(kDownlinkBaseVlanId);
  auto intfMap = swState->getInterfaces()->modify(&swState);
  for (const auto& [_, intfMap] : *intfMap) {
    for (auto iter = intfMap->begin(); iter != intfMap->end(); ++iter) {
      auto intf = iter->second;
      if (intf->getVlanID() >= downlinkBaseVlanId) {
        return intf->clone();
      }
    }
  }
  return nullptr;
}

void setTTLZeroCpuConfig(const HwAsic* hwAsic, cfg::SwitchConfig& config) {
  if (!hwAsic->isSupported(HwAsic::Feature::SAI_TTL0_PACKET_FORWARD_ENABLE)) {
    // don't configure if not supported
    return;
  }

  auto ttlRxReasonToQueue = cfg::PacketRxReasonToQueue();
  ttlRxReasonToQueue.rxReason() = cfg::PacketRxReason::TTL_0;
  ttlRxReasonToQueue.queueId() = 0;

  cfg::CPUTrafficPolicyConfig cpuConfig;
  cpuConfig.rxReasonToQueueOrderedList() = {std::move(ttlRxReasonToQueue)};
  config.cpuTrafficPolicy() = cpuConfig;
}

void setPortQueueSharedBytes(cfg::PortQueue& queue, bool isSai) {
  // Setting Shared Bytes for SAI is a no-op
  if (!isSai) {
    // Set sharedBytes for Low and Default Pri-Queue
    if (queue.id() == kCoppLowPriQueueId) {
      queue.sharedBytes() = kBcmCoppLowPriSharedBytes;
    } else if (queue.id() == kCoppDefaultPriQueueId) {
      queue.sharedBytes() = kBcmCoppDefaultPriSharedBytes;
    }
  }
}

void addNoActionAclForUnicastLinkLocal(
    const folly::CIDRNetwork& nw,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls) {
  cfg::AclEntry acl;
  auto dstIp = folly::to<std::string>(nw.first, "/", nw.second);
  acl.name() =
      folly::to<std::string>("cpuPolicing-CPU-Port-linkLocal-v6-", dstIp);

  acl.dstIp() = dstIp;
  acl.srcPort() = kCPUPort;
  acls.push_back(std::make_pair(acl, cfg::MatchAction{}));
}

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> defaultCpuAclsForSai(
    const HwAsic* hwAsic,
    cfg::SwitchConfig& /* unused */) {
  std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> acls;

  // TODO(daiweix): remove after ACL is fully supported by J3
  if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    return acls;
  }

  // Unicast link local from cpu
  addNoActionAclForUnicastLinkLocal(kIPv6LinkLocalUcastNetwork(), acls);

  // multicast link local dst ip
  addNoActionAclForNw(kIPv6LinkLocalMcastNetwork(), acls);

  // Link local IPv6 + DSCP 48 to high pri queue
  addHighPriAclForNwAndNetworkControlDscp(
      kIPv6LinkLocalMcastNetwork(),
      getCoppHighPriQueueId(hwAsic),
      getCpuActionType(hwAsic),
      acls,
      true /* isSai */);
  addHighPriAclForNwAndNetworkControlDscp(
      kIPv6LinkLocalUcastNetwork(),
      getCoppHighPriQueueId(hwAsic),
      getCpuActionType(hwAsic),
      acls,
      true /* isSai */);

  // unicast and multicast link local dst ip
  addMidPriAclForNw(
      kIPv6LinkLocalMcastNetwork(),
      getCpuActionType(hwAsic),
      acls,
      true /*isSai*/);
  // All fe80::/10 to mid pri queue
  addMidPriAclForNw(
      kIPv6LinkLocalUcastNetwork(),
      getCpuActionType(hwAsic),
      acls,
      true /*isSai*/);

  if (hwAsic->isSupported(HwAsic::Feature::ACL_METADATA_QUALIFER)) {
    // Unresolved route class ID to low pri queue
    addLowPriAclForUnresolvedRoutes(
        getCpuActionType(hwAsic), acls, true /*isSai*/);
  }

  return acls;
}

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> defaultCpuAclsForBcm(
    const HwAsic* hwAsic,
    cfg::SwitchConfig& config) {
  std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> acls;

  // Unicast link local from cpu
  addNoActionAclForUnicastLinkLocal(kIPv6LinkLocalUcastNetwork(), acls);

  // multicast link local dst ip
  addNoActionAclForNw(kIPv6LinkLocalMcastNetwork(), acls);

  bool isSai = false;
  // slow-protocols dst mac
  {
    cfg::AclEntry acl;
    acl.name() = "cpuPolicing-high-slow-protocols-mac";
    acl.dstMac() = LACPDU::kSlowProtocolsDstMac().toString();
    acls.emplace_back(
        acl,
        createQueueMatchAction(
            getCoppHighPriQueueId(hwAsic), isSai, getCpuActionType(hwAsic)));
  }

  // EAPOL
  {
    if (hwAsic->getAsicType() != cfg::AsicType::ASIC_TYPE_TRIDENT2) {
      cfg::AclEntry acl;
      acl.name() = "cpuPolicing-high-eapol";
      acl.dstMac() = "ff:ff:ff:ff:ff:ff";
      acl.etherType() = cfg::EtherType::EAPOL;
      acls.emplace_back(
          acl,
          createQueueMatchAction(
              getCoppHighPriQueueId(hwAsic), isSai, getCpuActionType(hwAsic)));
    }
  }

  // dstClassL3 w/ BGP port to high pri queue
  // Preffered L4 ports. Combine these with local interfaces
  // to put locally destined traffic to these ports to hi-pri queue.
  auto addHighPriDstClassL3BgpAcl = [&](bool isV4, bool isSrcPort) {
    cfg::AclEntry acl;
    acl.name() = folly::to<std::string>(
        "cpuPolicing-high-",
        isV4 ? "dstLocalIp4-" : "dstLocalIp6-",
        isSrcPort ? "srcPort:" : "dstPrt:",
        utility::kBgpPort);
    acl.lookupClassNeighbor() = isV4
        ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4
        : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6;

    if (isSrcPort) {
      acl.l4SrcPort() = utility::kBgpPort;
    } else {
      acl.l4DstPort() = utility::kBgpPort;
    }

    acls.emplace_back(
        acl,
        createQueueMatchAction(
            getCoppHighPriQueueId(hwAsic), isSai, getCpuActionType(hwAsic)));
  };
  addHighPriDstClassL3BgpAcl(true /*v4*/, true /*srcPort*/);
  addHighPriDstClassL3BgpAcl(true /*v4*/, false /*dstPort*/);
  addHighPriDstClassL3BgpAcl(false /*v6*/, true /*srcPort*/);
  addHighPriDstClassL3BgpAcl(false /*v6*/, false /*dstPort*/);

  // Dst IP local + DSCP 48 to high pri queue
  auto addHigPriLocalIpNetworkControlAcl = [&](bool isV4) {
    cfg::AclEntry acl;
    acl.name() = folly::to<std::string>(
        "cpuPolicing-high-",
        isV4 ? "dstLocalIp4" : "dstLocalIp6",
        "-network-control");
    acl.dscp() = 48;
    acl.lookupClassNeighbor() = isV4
        ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4
        : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6;

    acls.emplace_back(
        acl,
        createQueueMatchAction(
            getCoppHighPriQueueId(hwAsic), isSai, getCpuActionType(hwAsic)));
  };
  addHigPriLocalIpNetworkControlAcl(true);
  addHigPriLocalIpNetworkControlAcl(false);
  // Link local IPv6 + DSCP 48 to high pri queue
  auto addHighPriLinkLocalV6NetworkControlAcl =
      [&](const folly::CIDRNetwork& dstNetwork) {
        cfg::AclEntry acl;
        auto dstNetworkStr =
            folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
        acl.name() = folly::to<std::string>(
            "cpuPolicing-high-", dstNetworkStr, "-network-control");
        acl.dstIp() = dstNetworkStr;
        acl.dscp() = 48;
        acls.emplace_back(
            acl,
            createQueueMatchAction(
                getCoppHighPriQueueId(hwAsic),
                isSai,
                getCpuActionType(hwAsic)));
      };
  addHighPriLinkLocalV6NetworkControlAcl(kIPv6LinkLocalMcastNetwork());
  addHighPriLinkLocalV6NetworkControlAcl(kIPv6LinkLocalUcastNetwork());

  // add ACL to trap NDP solicit to high priority queue
  {
    cfg::AclEntry acl;
    auto dstNetwork = kIPv6NdpSolicitNetwork();
    auto dstNetworkStr =
        folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
    acl.name() = "cpuPolicing-high-ndp-solicit";
    acl.dstIp() = dstNetworkStr;
    acls.emplace_back(
        acl,
        createQueueMatchAction(
            getCoppHighPriQueueId(hwAsic), isSai, getCpuActionType(hwAsic)));
  }

  // Now steer traffic destined to this (local) interface IP
  // to mid pri queue. Note that we add this Acl entry *after*
  // (with a higher Acl ID) than locally destined protocol
  // traffic. Acl entries are matched in order, so we need to
  // go from more specific to less specific matches.
  auto addMidPriDstClassL3Acl = [&](bool isV4) {
    cfg::AclEntry acl;
    acl.name() = folly::to<std::string>(
        "cpuPolicing-mid-", isV4 ? "dstLocalIp4" : "dstLocalIp6");
    acl.lookupClassNeighbor() = isV4
        ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4
        : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6;

    acls.emplace_back(
        acl,
        createQueueMatchAction(
            utility::kCoppMidPriQueueId, isSai, getCpuActionType(hwAsic)));
  };
  addMidPriDstClassL3Acl(true);
  addMidPriDstClassL3Acl(false);

  // unicast and multicast link local dst ip
  addMidPriAclForNw(
      kIPv6LinkLocalMcastNetwork(), getCpuActionType(hwAsic), acls, isSai);
  // All fe80::/10 to mid pri queue
  addMidPriAclForNw(
      kIPv6LinkLocalUcastNetwork(), getCpuActionType(hwAsic), acls, isSai);

  // mpls no match
  {
    if (hwAsic->isSupported(HwAsic::Feature::MPLS)) {
      cfg::AclEntry acl;
      acl.name() = kMplsDestNoMatchAclName;
      acl.packetLookupResult() =
          cfg::PacketLookupResultType::PACKET_LOOKUP_RESULT_MPLS_NO_MATCH;
      std::vector<cfg::CounterType> counterTypes{cfg::CounterType::PACKETS};
      utility::addTrafficCounter(
          &config, kMplsDestNoMatchCounterName, counterTypes);
      auto queue = utility::kCoppLowPriQueueId;
      auto action =
          createQueueMatchAction(queue, isSai, getCpuActionType(hwAsic));
      action.counter() = kMplsDestNoMatchCounterName;
      acls.emplace_back(acl, action);
    }
  }
  return acls;
}

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>
defaultCpuAcls(const HwAsic* hwAsic, cfg::SwitchConfig& config, bool isSai) {
  return isSai ? defaultCpuAclsForSai(hwAsic, config)
               : defaultCpuAclsForBcm(hwAsic, config);
}

void addTrafficCounter(
    cfg::SwitchConfig* config,
    const std::string& counterName,
    std::optional<std::vector<cfg::CounterType>> counterTypes) {
  auto counter = cfg::TrafficCounter();
  *counter.name() = counterName;
  if (counterTypes.has_value()) {
    *counter.types() = counterTypes.value();
  } else {
    *counter.types() = {cfg::CounterType::PACKETS};
  }
  config->trafficCounters()->push_back(counter);
}

std::vector<cfg::PacketRxReasonToQueue> getCoppRxReasonToQueuesForSai(
    const HwAsic* hwAsic) {
  auto coppHighPriQueueId = utility::getCoppHighPriQueueId(hwAsic);
  ControlPlane::RxReasonToQueue rxReasonToQueues = {
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::ARP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::ARP_RESPONSE, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::NDP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::BGP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::BGPV6, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::CPU_IS_NHOP, kCoppMidPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::LACP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::TTL_1, kCoppLowPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::LLDP, kCoppMidPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::DHCP, kCoppMidPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::DHCPV6, kCoppMidPriQueueId),
  };

  // TODO(daiweix): remove after L4 port match is supported by J3 in 6.5.30
  if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    rxReasonToQueues = {
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::ARP, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::ARP_RESPONSE, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::NDP, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::BGP, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::CPU_IS_NHOP, kCoppMidPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::LACP, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::TTL_1, kCoppLowPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::LLDP, kCoppMidPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::DHCP, kCoppMidPriQueueId),
    };
  }

  if (hwAsic->isSupported(HwAsic::Feature::SAI_EAPOL_TRAP)) {
    rxReasonToQueues.push_back(ControlPlane::makeRxReasonToQueueEntry(
        cfg::PacketRxReason::EAPOL, coppHighPriQueueId));
  }

  if (hwAsic->isSupported(HwAsic::Feature::SAI_MPLS_TTL_1_TRAP)) {
    rxReasonToQueues.push_back(ControlPlane::makeRxReasonToQueueEntry(
        cfg::PacketRxReason::MPLS_TTL_1, kCoppLowPriQueueId));
  }

  if (hwAsic->isSupported(HwAsic::Feature::SAI_SAMPLEPACKET_TRAP)) {
    rxReasonToQueues.push_back(ControlPlane::makeRxReasonToQueueEntry(
        cfg::PacketRxReason::SAMPLEPACKET, kCoppLowPriQueueId));
  }

  // TODO: remove once CS00012311423 is fixed. Gate setting the L3 mtu error
  // trap on J2/J3 more specifically.
  if (hwAsic->isSupported(HwAsic::Feature::L3_MTU_ERROR_TRAP)) {
    rxReasonToQueues.push_back(ControlPlane::makeRxReasonToQueueEntry(
        cfg::PacketRxReason::L3_MTU_ERROR, kCoppLowPriQueueId));
  }

  return rxReasonToQueues;
}

std::vector<cfg::PacketRxReasonToQueue> getCoppRxReasonToQueuesForBcm(
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
    rxReasonToQueue.rxReason() = rxEntry.first;
    rxReasonToQueue.queueId() = rxEntry.second;
    rxReasonToQueues.push_back(rxReasonToQueue);
  }
  return rxReasonToQueues;
}

std::vector<cfg::PacketRxReasonToQueue> getCoppRxReasonToQueues(
    const HwAsic* hwAsic,
    bool isSai) {
  return isSai ? getCoppRxReasonToQueuesForSai(hwAsic)
               : getCoppRxReasonToQueuesForBcm(hwAsic);
}

cfg::MatchAction getToQueueActionForSai(
    const int queueId,
    const std::optional<cfg::ToCpuAction> toCpuAction) {
  cfg::MatchAction action;
  if (FLAGS_sai_user_defined_trap) {
    cfg::UserDefinedTrapAction userDefinedTrap;
    userDefinedTrap.queueId() = queueId;
    action.userDefinedTrap() = userDefinedTrap;
    // assume tc i maps to queue i for all i on sai switches
    cfg::SetTcAction setTc;
    setTc.tcValue() = queueId;
    action.setTc() = setTc;
  } else {
    cfg::QueueMatchAction queueAction;
    queueAction.queueId() = queueId;
    action.sendToQueue() = queueAction;
  }
  if (toCpuAction) {
    action.toCpuAction() = toCpuAction.value();
  }
  return action;
}

cfg::MatchAction getToQueueActionForBcm(
    const int queueId,
    const std::optional<cfg::ToCpuAction> toCpuAction) {
  cfg::MatchAction action;
  cfg::QueueMatchAction queueAction;
  queueAction.queueId() = queueId;
  action.sendToQueue() = queueAction;
  if (toCpuAction) {
    action.toCpuAction() = toCpuAction.value();
  }
  return action;
}

cfg::MatchAction getToQueueAction(
    const int queueId,
    bool isSai,
    const std::optional<cfg::ToCpuAction> toCpuAction) {
  return isSai ? getToQueueActionForSai(queueId, toCpuAction)
               : getToQueueActionForBcm(queueId, toCpuAction);
}

} // namespace facebook::fboss::utility
