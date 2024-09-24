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
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"

DECLARE_bool(enable_acl_table_group);

namespace {
constexpr int kDownlinkBaseVlanId = 2000;
constexpr uint32_t kCoppLowPriReservedBytes = 1040;
constexpr uint32_t kCoppDefaultPriReservedBytes = 1040;
constexpr uint32_t kBcmCoppLowPriSharedBytes = 10192;
constexpr uint32_t kDnxCoppLowMaxDynamicSharedBytes = 20 * 1024 * 1024;
constexpr uint32_t kDnxCoppMidMaxDynamicSharedBytes = 20 * 1024 * 1024;
constexpr uint32_t kBcmCoppDefaultPriSharedBytes = 10192;
const std::string kMplsDestNoMatchAclName = "cpuPolicing-mpls-dest-nomatch";
const std::string kMplsDestNoMatchCounterName = "mpls-dest-nomatch-counter";
} // unnamed namespace

namespace facebook::fboss::utility {

namespace {
std::unique_ptr<facebook::fboss::TxPacket> createUdpPktImpl(
    const AllocatePktFunc& allocatePkt,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIpAddress,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    uint8_t ttl,
    std::optional<uint8_t> dscp) {
  auto txPacket = utility::makeUDPTxPacket(
      allocatePkt,
      vlanId,
      srcMac,
      dstMac,
      srcIpAddress,
      dstIpAddress,
      l4SrcPort,
      l4DstPort,
      (dscp.has_value() ? dscp.value() : 48) << 2,
      ttl);

  return txPacket;
}

const auto kIPv6LinkLocalUcastAddress = folly::IPAddressV6("fe80::2");
const auto kNetworkControlDscp = 48;

} // namespace

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

uint16_t getCoppMidPriQueueId(const std::vector<const HwAsic*>& hwAsics) {
  auto hwAsic = checkSameAndGetAsic(hwAsics);
  if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    return kJ3CoppMidPriQueueId;
  }
  return kCoppMidPriQueueId;
}

uint16_t getCoppHighPriQueueId(const std::vector<const HwAsic*>& hwAsics) {
  auto hwAsic = checkSameAndGetAsic(hwAsics);
  return getCoppHighPriQueueId(hwAsic);
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
    uint32_t kbps;
    if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
      kbps = kCoppDnxLowPriKbitsPerSec;
    } else {
      kbps = getCoppQueueKbpsFromPps(hwAsic, pps);
    }
    portQueueRate.kbitsPerSec_ref() = getRange(0, kbps);
  }

  return portQueueRate;
}

void addCpuQueueConfig(
    cfg::SwitchConfig& config,
    const std::vector<const HwAsic*>& asics,
    bool isSai,
    bool setQueueRate) {
  auto hwAsic = checkSameAndGetAsic(asics);
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
  setPortQueueMaxDynamicSharedBytes(queue0, hwAsic);
  cpuQueues.push_back(queue0);

  if (!isSai) {
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
  }

  cfg::PortQueue queue2;
  queue2.id() = getCoppMidPriQueueId({hwAsic});
  queue2.name() = "cpuQueue-mid";
  queue2.streamType() = getCpuDefaultStreamType(hwAsic);
  queue2.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue2.weight() = kCoppMidPriWeight;
  setPortQueueMaxDynamicSharedBytes(queue2, hwAsic);
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
    const std::vector<const HwAsic*>& asics,
    bool isSai) {
  auto hwAsic = checkSameAndGetAsic(asics);
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
  auto rxReasonToQueues = getCoppRxReasonToQueues(asics, isSai);
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

void addEtherTypeToAcl(
    const HwAsic* hwAsic,
    const cfg::AclEntry& acl,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    const cfg::MatchAction& action,
    const std::vector<cfg::EtherType>& etherTypes) {
  if (hwAsic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
    for (auto etherType : etherTypes) {
      cfg::AclEntry newAcl = folly::copy(acl);
      if (etherType == cfg::EtherType::IPv4) {
        newAcl.name() = folly::to<std::string>(acl.name().value(), "-v4");
      } else {
        newAcl.name() = folly::to<std::string>(acl.name().value(), "-v6");
      }
      addEtherTypeToAcl(hwAsic, &newAcl, etherType);
      acls.push_back(std::make_pair(newAcl, action));
    }
  } else {
    acls.push_back(std::make_pair(acl, action));
  }
}

void addNoActionAclForNw(
    const HwAsic* hwAsic,
    const folly::CIDRNetwork& nw,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls) {
  cfg::AclEntry acl;
  auto dstIp = folly::to<std::string>(nw.first, "/", nw.second);
  acl.name() = folly::to<std::string>("cpuPolicing-CPU-Port-Mcast-v6-", dstIp);

  acl.dstIp() = dstIp;
  acl.srcPort() = kCPUPort;
  utility::addEtherTypeToAcl(
      hwAsic,
      acl,
      acls,
      cfg::MatchAction{},
      {nw.first.isV6() ? cfg::EtherType::IPv6 : cfg::EtherType::IPv4});
}

void addHighPriAclForNwAndNetworkControlDscp(
    const HwAsic* hwAsic,
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
  utility::addEtherTypeToAcl(
      hwAsic,
      acl,
      acls,
      createQueueMatchAction(highPriQueueId, isSai, toCpuAction),
      {dstNetwork.first.isV6() ? cfg::EtherType::IPv6 : cfg::EtherType::IPv4});
}

void addMidPriAclForNw(
    const HwAsic* hwAsic,
    const folly::CIDRNetwork& dstNetwork,
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai,
    int midPriQueueId) {
  cfg::AclEntry acl;
  auto dstIp = folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
  acl.name() = folly::to<std::string>("cpuPolicing-mid-", dstIp);
  acl.dstIp() = dstIp;
  utility::addEtherTypeToAcl(
      hwAsic,
      acl,
      acls,
      createQueueMatchAction(midPriQueueId, isSai, toCpuAction),
      {dstNetwork.first.isV6() ? cfg::EtherType::IPv6 : cfg::EtherType::IPv4});
}

void addHighPriAclForMyIPNetworkControl(
    const HwAsic* hwAsic,
    cfg::ToCpuAction toCpuAction,
    int highPriQueueId,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai) {
  cfg::AclEntry acl;
  acl.name() =
      folly::to<std::string>("cpuPolicing-high-myip-network-control-acl");
  acl.lookupClassRoute() = cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1;
  acl.dscp() = 48;
  addEtherTypeToAcl(
      hwAsic,
      acl,
      acls,
      createQueueMatchAction(highPriQueueId, isSai, toCpuAction),
      {cfg::EtherType::IPv4, cfg::EtherType::IPv6});
}

void addHighPriAclForBgp(
    const HwAsic* hwAsic,
    cfg::ToCpuAction toCpuAction,
    int highPriQueueId,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai) {
  cfg::AclEntry dstPortAcl;
  dstPortAcl.name() =
      folly::to<std::string>("cpuPolicing-high-dst-port-bgp-acl");
  dstPortAcl.l4DstPort() = utility::kBgpPort;
  dstPortAcl.lookupClassRoute() = cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1;
  dstPortAcl.proto() = 6; // tcp
  addEtherTypeToAcl(
      hwAsic,
      dstPortAcl,
      acls,
      createQueueMatchAction(highPriQueueId, isSai, toCpuAction),
      {cfg::EtherType::IPv4, cfg::EtherType::IPv6});

  cfg::AclEntry srcPortAcl;
  srcPortAcl.name() =
      folly::to<std::string>("cpuPolicing-high-src-port-bgp-acl");
  srcPortAcl.l4SrcPort() = utility::kBgpPort;
  srcPortAcl.lookupClassRoute() = cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1;
  srcPortAcl.proto() = 6; // tcp
  addEtherTypeToAcl(
      hwAsic,
      srcPortAcl,
      acls,
      createQueueMatchAction(highPriQueueId, isSai, toCpuAction),
      {cfg::EtherType::IPv4, cfg::EtherType::IPv6});
}

void addHighPriAclForArp(
    cfg::ToCpuAction toCpuAction,
    int highPriQueueId,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai) {
  cfg::AclEntry acl1;
  acl1.etherType() = cfg::EtherType::ARP;
  acl1.ipType() = cfg::IpType::ARP_REQUEST;
  acl1.name() = folly::to<std::string>("cpuPolicing-high-arp-request-acl");
  auto action = createQueueMatchAction(highPriQueueId, isSai, toCpuAction);
  acls.push_back(std::make_pair(acl1, action));
  cfg::AclEntry acl2;
  acl2.etherType() = cfg::EtherType::ARP;
  acl2.ipType() = cfg::IpType::ARP_REPLY;
  acl2.name() = folly::to<std::string>("cpuPolicing-high-arp-reply-acl");
  acls.push_back(std::make_pair(acl2, action));
}

void addHighPriAclForLacp(
    cfg::ToCpuAction toCpuAction,
    int highPriQueueId,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai) {
  cfg::AclEntry acl;
  acl.etherType() = cfg::EtherType::LACP;
  acl.dstMac() = LACPDU::kSlowProtocolsDstMac().toString();
  acl.name() = folly::to<std::string>("cpuPolicing-high-lacp-acl");
  auto action = createQueueMatchAction(highPriQueueId, isSai, toCpuAction);
  acls.push_back(std::make_pair(acl, action));
}

void addMidPriAclForLldp(
    cfg::ToCpuAction toCpuAction,
    int midPriQueueId,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai) {
  cfg::AclEntry acl;
  acl.etherType() = cfg::EtherType::LLDP;
  acl.dstMac() = LldpManager::LLDP_DEST_MAC.toString();
  acl.name() = folly::to<std::string>("cpuPolicing-mid-lldp-acl");
  auto action = createQueueMatchAction(midPriQueueId, isSai, toCpuAction);
  acls.push_back(std::make_pair(acl, action));
}

void addMidPriAclForIp2Me(
    const HwAsic* hwAsic,
    cfg::ToCpuAction toCpuAction,
    int midPriQueueId,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai) {
  cfg::AclEntry acl;
  acl.lookupClassRoute() = cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1;
  acl.name() = folly::to<std::string>("cpuPolicing-mid-ip2me-acl");
  addEtherTypeToAcl(
      hwAsic,
      acl,
      acls,
      createQueueMatchAction(midPriQueueId, isSai, toCpuAction),
      {cfg::EtherType::IPv4, cfg::EtherType::IPv6});
}

void addLowPriAclForUnresolvedRoutes(
    const HwAsic* hwAsic,
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls,
    bool isSai) {
  cfg::AclEntry acl;
  acl.name() = folly::to<std::string>("cpu-unresolved-route-acl");
  acl.lookupClassRoute() = cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2;
  addEtherTypeToAcl(
      hwAsic,
      acl,
      acls,
      createQueueMatchAction(utility::kCoppLowPriQueueId, isSai, toCpuAction),
      {cfg::EtherType::IPv4, cfg::EtherType::IPv6});
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

void setTTLZeroCpuConfig(
    const std::vector<const HwAsic*>& asics,
    cfg::SwitchConfig& config) {
  auto hwAsic = checkSameAndGetAsic(asics);
  if (!hwAsic->isSupported(HwAsic::Feature::SAI_TTL0_PACKET_FORWARD_ENABLE)) {
    // don't configure if not supported
    return;
  }

  std::vector<cfg::PacketRxReasonToQueue> rxReasons;
  bool addTtlRxReason = true;
  if (config.cpuTrafficPolicy().has_value() &&
      config.cpuTrafficPolicy()->rxReasonToQueueOrderedList().has_value() &&
      config.cpuTrafficPolicy()->rxReasonToQueueOrderedList()->size()) {
    for (auto rxReasonAndQueue :
         *config.cpuTrafficPolicy()->rxReasonToQueueOrderedList()) {
      if (rxReasonAndQueue.rxReason() == cfg::PacketRxReason::TTL_0) {
        addTtlRxReason = false;
      }
      rxReasons.push_back(rxReasonAndQueue);
    }
  }
  if (addTtlRxReason) {
    auto ttlRxReasonToQueue = cfg::PacketRxReasonToQueue();
    ttlRxReasonToQueue.rxReason() = cfg::PacketRxReason::TTL_0;
    ttlRxReasonToQueue.queueId() = 0;

    rxReasons.push_back(ttlRxReasonToQueue);
  }
  cfg::CPUTrafficPolicyConfig cpuConfig;

  if (config.cpuTrafficPolicy().has_value()) {
    auto origCpuTrafficPolicy = *config.cpuTrafficPolicy();
    if (origCpuTrafficPolicy.trafficPolicy()) {
      cpuConfig.trafficPolicy() = *origCpuTrafficPolicy.trafficPolicy();
    }
  }
  cpuConfig.rxReasonToQueueOrderedList() = rxReasons;
  config.cpuTrafficPolicy() = cpuConfig;
}

void excludeTTL1TrapConfig(cfg::SwitchConfig& config) {
  std::vector<cfg::PacketRxReasonToQueue> rxReasons;
  // Exclude TTL_1 trap since on some devices we disable it
  // to set up data plane loops
  CHECK(config.cpuTrafficPolicy().has_value());
  CHECK(config.cpuTrafficPolicy()->rxReasonToQueueOrderedList().has_value());
  if (config.cpuTrafficPolicy()->rxReasonToQueueOrderedList()->size()) {
    for (auto rxReasonAndQueue :
         *config.cpuTrafficPolicy()->rxReasonToQueueOrderedList()) {
      if (*rxReasonAndQueue.rxReason() != cfg::PacketRxReason::TTL_1) {
        rxReasons.push_back(rxReasonAndQueue);
      }
    }
  }
  config.cpuTrafficPolicy()->rxReasonToQueueOrderedList() = rxReasons;
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

void setPortQueueMaxDynamicSharedBytes(
    cfg::PortQueue& queue,
    const HwAsic* hwAsic) {
  if (hwAsic->isSupported(HwAsic::Feature::CPU_VOQ_BUFFER_PROFILE)) {
    if (queue.id() == kCoppLowPriQueueId) {
      queue.maxDynamicSharedBytes() = kDnxCoppLowMaxDynamicSharedBytes;
    } else if (queue.id() == getCoppMidPriQueueId({hwAsic})) {
      queue.maxDynamicSharedBytes() = kDnxCoppMidMaxDynamicSharedBytes;
    }
  }
}

void addNoActionAclForUnicastLinkLocal(
    const HwAsic* hwAsic,
    const folly::CIDRNetwork& nw,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls) {
  cfg::AclEntry acl;
  auto dstIp = folly::to<std::string>(nw.first, "/", nw.second);
  acl.name() =
      folly::to<std::string>("cpuPolicing-CPU-Port-linkLocal-v6-", dstIp);

  acl.dstIp() = dstIp;
  acl.srcPort() = kCPUPort;
  utility::addEtherTypeToAcl(
      hwAsic,
      acl,
      acls,
      cfg::MatchAction{},
      {nw.first.isV6() ? cfg::EtherType::IPv6 : cfg::EtherType::IPv4});
}

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> defaultCpuAclsForSai(
    const HwAsic* hwAsic,
    cfg::SwitchConfig& /* unused */) {
  std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> acls;

  // Unicast link local from cpu
  addNoActionAclForUnicastLinkLocal(hwAsic, kIPv6LinkLocalUcastNetwork(), acls);

  // multicast link local dst ip
  addNoActionAclForNw(hwAsic, kIPv6LinkLocalMcastNetwork(), acls);

  // Link local IPv6 + DSCP 48 to high pri queue
  addHighPriAclForNwAndNetworkControlDscp(
      hwAsic,
      kIPv6LinkLocalMcastNetwork(),
      getCoppHighPriQueueId(hwAsic),
      getCpuActionType(hwAsic),
      acls,
      true /* isSai */);
  addHighPriAclForNwAndNetworkControlDscp(
      hwAsic,
      kIPv6LinkLocalUcastNetwork(),
      getCoppHighPriQueueId(hwAsic),
      getCpuActionType(hwAsic),
      acls,
      true /* isSai */);

  // unicast and multicast link local dst ip
  addMidPriAclForNw(
      hwAsic,
      kIPv6LinkLocalMcastNetwork(),
      getCpuActionType(hwAsic),
      acls,
      true /*isSai*/,
      getCoppMidPriQueueId({hwAsic}));
  // All fe80::/10 to mid pri queue
  addMidPriAclForNw(
      hwAsic,
      kIPv6LinkLocalUcastNetwork(),
      getCpuActionType(hwAsic),
      acls,
      true /*isSai*/,
      getCoppMidPriQueueId({hwAsic}));

  if (hwAsic->isSupported(HwAsic::Feature::ACL_METADATA_QUALIFER)) {
    addHighPriAclForMyIPNetworkControl(
        hwAsic,
        cfg::ToCpuAction::TRAP,
        getCoppHighPriQueueId(hwAsic),
        acls,
        true /*isSai*/);
    /*
     * Unresolved route class ID to low pri queue.
     * For unresolved route ACL, both the hostif trap and the ACL will
     * be hit on TAJO and 2 packets will be punted to CPU.
     * Do not rely on getCpuActionType but explicitly configure
     * the cpu action to TRAP. Connected subnet route has the same class ID
     * and also goes to low pri queue
     */
    addLowPriAclForUnresolvedRoutes(
        hwAsic, cfg::ToCpuAction::TRAP, acls, true /*isSai*/);

    if (hwAsic->isSupported(HwAsic::Feature::NO_RX_REASON_TRAP)) {
      addHighPriAclForArp(

          cfg::ToCpuAction::TRAP, getCoppHighPriQueueId(hwAsic), acls, true);
      addHighPriAclForBgp(
          hwAsic,
          cfg::ToCpuAction::TRAP,
          getCoppHighPriQueueId(hwAsic),
          acls,
          true);
      addMidPriAclForIp2Me(
          hwAsic,
          cfg::ToCpuAction::TRAP,
          getCoppMidPriQueueId({hwAsic}),
          acls,
          true);
      addHighPriAclForLacp(
          cfg::ToCpuAction::TRAP, getCoppHighPriQueueId(hwAsic), acls, true);
      addMidPriAclForLldp(
          cfg::ToCpuAction::TRAP, getCoppMidPriQueueId({hwAsic}), acls, true);
    }
  }

  return acls;
}

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> defaultCpuAclsForBcm(
    const HwAsic* hwAsic,
    cfg::SwitchConfig& config) {
  std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> acls;

  // Unicast link local from cpu
  addNoActionAclForUnicastLinkLocal(hwAsic, kIPv6LinkLocalUcastNetwork(), acls);

  // multicast link local dst ip
  addNoActionAclForNw(hwAsic, kIPv6LinkLocalMcastNetwork(), acls);

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
        ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1
        : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2;

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
        ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1
        : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2;

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
        ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1
        : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2;

    acls.emplace_back(
        acl,
        createQueueMatchAction(
            utility::getCoppMidPriQueueId({hwAsic}),
            isSai,
            getCpuActionType(hwAsic)));
  };
  addMidPriDstClassL3Acl(true);
  addMidPriDstClassL3Acl(false);

  // unicast and multicast link local dst ip
  addMidPriAclForNw(
      hwAsic,
      kIPv6LinkLocalMcastNetwork(),
      getCpuActionType(hwAsic),
      acls,
      isSai,
      getCoppMidPriQueueId({hwAsic}));
  // All fe80::/10 to mid pri queue
  addMidPriAclForNw(
      hwAsic,
      kIPv6LinkLocalUcastNetwork(),
      getCpuActionType(hwAsic),
      acls,
      isSai,
      getCoppMidPriQueueId({hwAsic}));

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
  auto coppMidPriQueueId = utility::getCoppMidPriQueueId({hwAsic});
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
          cfg::PacketRxReason::CPU_IS_NHOP, coppMidPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::LACP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::TTL_1, kCoppLowPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::LLDP, coppMidPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::DHCP, coppMidPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::DHCPV6, coppMidPriQueueId),
  };

  if (hwAsic->isSupported(HwAsic::Feature::NO_RX_REASON_TRAP)) {
    // TODO(daiweix): remove these rx reason traps and replace them by ACLs
    rxReasonToQueues = {
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::NDP, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::TTL_1, kCoppLowPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::DHCP, coppMidPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::DHCPV6, coppMidPriQueueId),
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
  auto coppMidPriQueueId = utility::getCoppMidPriQueueId({hwAsic});
  std::vector<std::pair<cfg::PacketRxReason, uint16_t>>
      rxReasonToQueueMappings = {
          std::pair(cfg::PacketRxReason::ARP, coppHighPriQueueId),
          std::pair(cfg::PacketRxReason::DHCP, coppMidPriQueueId),
          std::pair(cfg::PacketRxReason::BPDU, coppMidPriQueueId),
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
    const std::vector<const HwAsic*>& hwAsics,
    bool isSai) {
  auto hwAsic = checkSameAndGetAsic(hwAsics);
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

CpuPortStats getLatestCpuStats(SwSwitch* sw, SwitchID switchId) {
  // Stats collection from SwSwitch is async, wait for stats
  // being available before returning here.
  CpuPortStats cpuStats;
  checkWithRetry(
      [&cpuStats, switchId, sw]() {
        auto switchStats = sw->getHwSwitchStatsExpensive();
        if (switchStats.find(switchId) == switchStats.end()) {
          return false;
        }
        cpuStats = *switchStats.at(switchId).cpuPortStats();
        return !cpuStats.queueInPackets_()->empty();
      },
      120,
      std::chrono::milliseconds(1000),
      " fetch port stats");
  return cpuStats;
}

uint64_t getCpuQueueInPackets(SwSwitch* sw, SwitchID switchId, int queueId) {
  auto cpuPortStats = getLatestCpuStats(sw, switchId);
  return cpuPortStats.queueInPackets_()->find(queueId) ==
          cpuPortStats.queueInPackets_()->end()
      ? 0
      : cpuPortStats.queueInPackets_()->at(queueId);
}

template <typename SwitchT>
uint64_t getQueueOutPacketsWithRetry(
    SwitchT* switchPtr,
    SwitchID switchId,
    int queueId,
    int retryTimes,
    uint64_t expectedNumPkts,
    int postMatchRetryTimes) {
  uint64_t outPkts = 0;
  const HwAsic* asic;
  if constexpr (std::is_same_v<SwitchT, SwSwitch>) {
    asic = static_cast<SwSwitch*>(switchPtr)->getHwAsicTable()->getHwAsicIf(
        switchId);
  } else {
    asic = static_cast<HwSwitch*>(switchPtr)->getPlatform()->getAsic();
  }

  do {
    for (auto i = 0; i <= utility::getCoppHighPriQueueId(asic); i++) {
      auto qOutPkts =
          getCpuQueueOutPacketsAndBytes(switchPtr, i, switchId).first;
      XLOG(DBG2) << "QueueID: " << i << " qOutPkts: " << qOutPkts;
    }

    outPkts = getCpuQueueOutPacketsAndBytes(switchPtr, queueId, switchId).first;
    if (retryTimes == 0 || (outPkts >= expectedNumPkts)) {
      break;
    }

    /*
     * Post warmboot, the packet always gets processed by the right CPU
     * queue (as per ACL/rxreason etc.) but sometimes it is delayed.
     * Retrying a few times to avoid test noise.
     */
    XLOG(DBG0) << "Retry...";
    /* sleep override */
    sleep(1);
  } while (retryTimes-- > 0);

  while ((outPkts == expectedNumPkts) && postMatchRetryTimes--) {
    outPkts = getCpuQueueOutPacketsAndBytes(switchPtr, queueId, switchId).first;
  }

  return outPkts;
}

std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
    const HwSwitch* hwSwitch,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIpAddress,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    uint8_t ttl,
    std::optional<uint8_t> dscp) {
  return createUdpPktImpl(
      [hwSwitch](uint32_t size) { return hwSwitch->allocatePacket(size); },
      vlanId,
      srcMac,
      dstMac,
      srcIpAddress,
      dstIpAddress,
      l4SrcPort,
      l4DstPort,
      ttl,
      dscp);
}

std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
    TestEnsembleIf* ensemble,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIpAddress,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    uint8_t ttl,
    std::optional<uint8_t> dscp) {
  return createUdpPktImpl(
      [ensemble](uint32_t size) { return ensemble->allocatePacket(size); },
      vlanId,
      srcMac,
      dstMac,
      srcIpAddress,
      dstIpAddress,
      l4SrcPort,
      l4DstPort,
      ttl,
      dscp);
}

std::pair<uint64_t, uint64_t>
getCpuQueueOutPacketsAndBytes(SwSwitch* sw, int queueId, SwitchID switchId) {
  HwPortStats stats = *getLatestCpuStats(sw, switchId).portStats_();
  return getCpuQueueOutPacketsAndBytes(stats, queueId);
}

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* hw,
    int queueId,
    SwitchID /* switchId */) {
  hw->updateStats();
  HwPortStats stats = *hw->getCpuPortStats().portStats_();
  return getCpuQueueOutPacketsAndBytes(stats, queueId);
}

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwPortStats& stats,
    int queueId) {
  auto queueIter = stats.queueOutPackets_()->find(queueId);
  auto outPackets =
      (queueIter != stats.queueOutPackets_()->end()) ? queueIter->second : 0;
  queueIter = stats.queueOutBytes_()->find(queueId);
  auto outBytes =
      (queueIter != stats.queueOutBytes_()->end()) ? queueIter->second : 0;
  return std::pair(outPackets, outBytes);
}

template uint64_t getQueueOutPacketsWithRetry<SwSwitch>(
    SwSwitch* switchPtr,
    SwitchID switchId,
    int queueId,
    int retryTimes,
    uint64_t expectedNumPkts,
    int postMatchRetryTimes);

template uint64_t getQueueOutPacketsWithRetry<HwSwitch>(
    HwSwitch* switchPtr,
    SwitchID switchId,
    int queueId,
    int retryTimes,
    uint64_t expectedNumPkts,
    int postMatchRetryTimes);

template <typename SwitchT>
void sendAndVerifyPkts(
    SwitchT* switchPtr,
    SwitchID switchId,
    std::shared_ptr<SwitchState> swState,
    const folly::IPAddress& destIp,
    uint16_t destPort,
    uint8_t queueId,
    PortID srcPort,
    uint8_t trafficClass) {
  auto sendPkts = [&] {
    auto vlanId = utility::firstVlanID(swState);
    auto intfMac = utility::getFirstInterfaceMac(swState);
    utility::sendTcpPkts(
        switchPtr,
        1 /*numPktsToSend*/,
        vlanId,
        intfMac,
        destIp,
        utility::kNonSpecialPort1,
        destPort,
        srcPort,
        trafficClass);
  };

  sendPktAndVerifyCpuQueue(switchPtr, switchId, queueId, sendPkts, 1);
}

/*
 * Pick a common copp Acl (link local + NC) and run dataplane test
 * to verify whether a common COPP acl is being hit.
 * TODO: Enhance this to cover every copp invariant acls.
 * Implement a similar function to cover all rxreasons invariant as well
 */
template <typename SwitchT>
void verifyCoppAcl(
    SwitchT* switchPtr,
    SwitchID switchId,
    const HwAsic* hwAsic,
    std::shared_ptr<SwitchState> swState,
    PortID srcPort) {
  XLOG(DBG2) << "Verifying Copp ACL";
  sendAndVerifyPkts(
      switchPtr,
      switchId,
      swState,
      kIPv6LinkLocalUcastAddress,
      utility::kNonSpecialPort2,
      utility::getCoppHighPriQueueId(hwAsic),
      srcPort,
      kNetworkControlDscp);
}

template <typename SwitchT>
void verifyCoppInvariantHelper(
    SwitchT* switchPtr,
    SwitchID switchId,
    const HwAsic* hwAsic,
    std::shared_ptr<SwitchState> swState,
    PortID srcPort) {
  auto intf = getEligibleInterface(swState);
  if (!intf) {
    throw FbossError(
        "No eligible uplink/downlink interfaces in config to verify COPP invariant");
  }
  for (auto iter : std::as_const(*intf->getAddresses())) {
    auto destIp = folly::IPAddress(iter.first);
    if (destIp.isLinkLocal()) {
      // three elements in the address vector: ipv4, ipv6 and a link local one
      // if the address qualifies as link local, it will loop back to the queue
      // again, adding an extra packet to the queue and failing the verification
      // thus, we skip the last one and only send BGP packets to v4 and v6 addr
      continue;
    }
    sendAndVerifyPkts(
        switchPtr,
        switchId,
        swState,
        destIp,
        utility::kBgpPort,
        utility::getCoppHighPriQueueId(hwAsic),
        srcPort);
  }
  auto addrs = intf->getAddressesCopy();
  sendAndVerifyPkts(
      switchPtr,
      switchId,
      swState,
      addrs.begin()->first,
      utility::kNonSpecialPort2,
      utility::getCoppMidPriQueueId({hwAsic}),
      srcPort);

  verifyCoppAcl(switchPtr, switchId, hwAsic, swState, srcPort);
}

CpuPortStats getCpuPortStats(SwSwitch* sw, SwitchID switchId) {
  std::map<int, CpuPortStats> cpuStats;
  sw->getAllCpuPortStats(cpuStats);
  if (cpuStats.find(switchId) == cpuStats.end()) {
    throw FbossError("No cpu port stats found for switchId: ", switchId);
  }
  return cpuStats.at(switchId);
}

/*
 * Takes a SwitchConfig and returns a map of queue IDs to DSCPs.
 * Particularly useful in verifyQueueMappings, where we don't have a guarantee
 * of what the QoS policies look like and we can't rely on something like
 * kOlympicQueueToDscp().
 */
std::map<int, std::vector<uint8_t>> getOlympicQosMaps(
    const cfg::SwitchConfig& config) {
  std::map<int, std::vector<uint8_t>> queueToDscp;

  for (const auto& qosPolicy : *config.qosPolicies()) {
    const auto& qosName = qosPolicy.get_name();
    XLOG(DBG2) << "Iterating over QoS policies: found qosPolicy " << qosName;

    // Optional thrift field access
    if (const auto& qosMap = qosPolicy.qosMap()) {
      const auto& dscpMaps = *qosMap->dscpMaps();

      for (const auto& dscpMap : dscpMaps) {
        auto queueId = dscpMap.get_internalTrafficClass();
        // Internally (i.e. in thrift), the mapping is implemented as a
        // map<int16_t, vector<int8_t>>; however, in functions like
        // verifyQueueMapping in HwTestQosUtils, the argument used is of the
        // form map<int, uint8_t>.
        // Trying to assign vector<uint8_t> to a vector<int8_t> makes the STL
        // unhappy, so we can just loop through and construct one on our own.
        std::vector<uint8_t> dscps;
        for (auto val : *dscpMap.fromDscpToTrafficClass()) {
          dscps.push_back((uint8_t)val);
        }
        queueToDscp[(int)queueId] = std::move(dscps);
      }
    } else {
      XLOG(ERR) << "qosMap not found in qosPolicy: " << qosName;
    }
  }

  return queueToDscp;
}

uint32_t getDnxCoppMaxDynamicSharedBytes(uint16_t queueId) {
  if (queueId == kCoppLowPriQueueId) {
    return kDnxCoppLowMaxDynamicSharedBytes;
  }
  if (queueId == kJ3CoppMidPriQueueId) {
    return kDnxCoppMidMaxDynamicSharedBytes;
  }
  // should not reach here
  XLOG(FATAL) << "no max dynamic shared bytes set for queue " << queueId;
  return 0;
}

AgentConfig setTTL0PacketForwardingEnableConfig(
    SwSwitch* sw,
    AgentConfig& agentConfig) {
  cfg::AgentConfig testConfig = agentConfig.thrift;
  cfg::SwitchConfig swConfig = testConfig.sw().value();
  // Setup TTL0 CPU queue
  utility::setTTLZeroCpuConfig(sw->getHwAsicTable()->getL3Asics(), swConfig);
  auto newAgentConfig = AgentConfig(
      testConfig,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(testConfig));
  return newAgentConfig;
}

template void sendAndVerifyPkts<SwSwitch>(
    SwSwitch* switchPtr,
    SwitchID switchId,
    std::shared_ptr<SwitchState> swState,
    const folly::IPAddress& destIp,
    uint16_t destPort,
    uint8_t queueId,
    PortID srcPort,
    uint8_t trafficClass);

template void sendAndVerifyPkts<HwSwitch>(
    HwSwitch* switchPtr,
    SwitchID switchId,
    std::shared_ptr<SwitchState> swState,
    const folly::IPAddress& destIp,
    uint16_t destPort,
    uint8_t queueId,
    PortID srcPort,
    uint8_t trafficClass);

template void verifyCoppInvariantHelper<SwSwitch>(
    SwSwitch* switchPtr,
    SwitchID switchId,
    const HwAsic* hwAsic,
    std::shared_ptr<SwitchState> swState,
    PortID srcPort);

template void verifyCoppInvariantHelper<HwSwitch>(
    HwSwitch* switchPtr,
    SwitchID switchId,
    const HwAsic* hwAsic,
    std::shared_ptr<SwitchState> swState,
    PortID srcPort);
} // namespace facebook::fboss::utility
