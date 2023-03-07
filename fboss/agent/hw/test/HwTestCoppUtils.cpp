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
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/ResourceLibUtil.h"

namespace {
constexpr uint32_t kCoppLowPriReservedBytes = 1040;
constexpr uint32_t kCoppDefaultPriReservedBytes = 1040;
} // unnamed namespace

namespace facebook::fboss::utility {

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
    case cfg::AsicType::ASIC_TYPE_INDUS:
      return 7;
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_RAMON:
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
    case cfg::AsicType::ASIC_TYPE_INDUS:
      return cfg::ToCpuAction::COPY;
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_RAMON:
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
  setPortQueueSharedBytes(queue0);
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
  setPortQueueSharedBytes(queue1);
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
    const HwAsic* hwAsic) {
  auto cpuAcls = utility::defaultCpuAcls(hwAsic, config);
  // insert cpu acls into global acl field
  int curNumAcls = config.acls()->size();
  config.acls()->resize(curNumAcls + cpuAcls.size());
  for (int i = 0; i < cpuAcls.size(); i++) {
    config.acls()[curNumAcls + i] = cpuAcls[i].first;
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
  auto rxReasonToQueues = getCoppRxReasonToQueues(hwAsic);
  if (rxReasonToQueues.size()) {
    cpuConfig.rxReasonToQueueOrderedList() = rxReasonToQueues;
  }
  config.cpuTrafficPolicy() = cpuConfig;
}

cfg::MatchAction createQueueMatchAction(
    int queueId,
    cfg::ToCpuAction toCpuAction) {
  cfg::MatchAction action;
  cfg::QueueMatchAction queueAction;
  queueAction.queueId() = queueId;
  action.sendToQueue() = queueAction;

  switch (toCpuAction) {
    case cfg::ToCpuAction::COPY:
      action.toCpuAction() = cfg::ToCpuAction::COPY;
      break;
    case cfg::ToCpuAction::TRAP:
      action.toCpuAction() = cfg::ToCpuAction::TRAP;
      break;
    default:
      throw FbossError("Unsupported CounterType for ACL");
  }

  return action;
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
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls) {
  cfg::AclEntry acl;
  auto dstNetworkStr =
      folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
  acl.name() = folly::to<std::string>(
      "cpuPolicing-high-", dstNetworkStr, "-network-control");
  acl.dstIp() = dstNetworkStr;
  acl.dscp() = 48;
  acls.push_back(
      std::make_pair(acl, createQueueMatchAction(highPriQueueId, toCpuAction)));
}

void addMidPriAclForNw(
    const folly::CIDRNetwork& dstNetwork,
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls) {
  cfg::AclEntry acl;
  auto dstIp = folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
  acl.name() = folly::to<std::string>("cpuPolicing-mid-", dstIp);
  acl.dstIp() = dstIp;

  acls.push_back(std::make_pair(
      acl, createQueueMatchAction(utility::kCoppMidPriQueueId, toCpuAction)));
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
  auto txPacket = utility::makeUDPTxPacket(
      hwSwitch,
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

uint64_t getQueueOutPacketsWithRetry(
    HwSwitch* hwSwitch,
    int queueId,
    int retryTimes,
    uint64_t expectedNumPkts,
    int postMatchRetryTimes) {
  uint64_t outPkts = 0, outBytes = 0;
  do {
    std::tie(outPkts, outBytes) =
        getCpuQueueOutPacketsAndBytes(hwSwitch, queueId);
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
    std::tie(outPkts, outBytes) =
        getCpuQueueOutPacketsAndBytes(hwSwitch, queueId);
  }

  return outPkts;
}

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId) {
  auto hwPortStats = getCpuQueueStats(hwSwitch);
  auto queueIter = hwPortStats.queueOutPackets_()->find(queueId);
  auto outPackets = (queueIter != hwPortStats.queueOutPackets_()->end())
      ? queueIter->second
      : 0;
  queueIter = hwPortStats.queueOutBytes_()->find(queueId);
  auto outBytes = (queueIter != hwPortStats.queueOutBytes_()->end())
      ? queueIter->second
      : 0;
  return std::pair(outPackets, outBytes);
}

std::pair<uint64_t, uint64_t> getCpuQueueOutDiscardPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId) {
  auto hwPortStats = getCpuQueueStats(hwSwitch);
  auto queueIter = hwPortStats.queueOutDiscardPackets_()->find(queueId);
  auto outDiscardPackets =
      (queueIter != hwPortStats.queueOutDiscardPackets_()->end())
      ? queueIter->second
      : 0;
  queueIter = hwPortStats.queueOutDiscardBytes_()->find(queueId);
  auto outDiscardBytes =
      (queueIter != hwPortStats.queueOutDiscardBytes_()->end())
      ? queueIter->second
      : 0;
  return std::pair(outDiscardPackets, outDiscardBytes);
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

void sendAndVerifyPkts(
    HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> swState,
    const folly::IPAddress& destIp,
    uint16_t destPort,
    uint8_t queueId,
    PortID srcPort) {
  auto sendPkts = [&] {
    auto vlanId = utility::firstVlanID(swState);
    auto intfMac = utility::getFirstInterfaceMac(swState);
    utility::sendTcpPkts(
        hwSwitch,
        1 /*numPktsToSend*/,
        vlanId,
        intfMac,
        destIp,
        utility::kNonSpecialPort1,
        destPort,
        srcPort);
  };

  sendPktAndVerifyCpuQueue(hwSwitch, queueId, sendPkts, 1);
}

void verifyCoppInvariantHelper(
    HwSwitch* hwSwitch,
    const HwAsic* hwAsic,
    std::shared_ptr<SwitchState> swState,
    PortID srcPort) {
  auto intf = std::as_const(*swState->getInterfaces()->cbegin()).second;
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
        hwSwitch,
        swState,
        destIp,
        utility::kBgpPort,
        utility::getCoppHighPriQueueId(hwAsic),
        srcPort);
  }
  auto addrs = intf->getAddressesCopy();
  sendAndVerifyPkts(
      hwSwitch,
      swState,
      addrs.begin()->first,
      utility::kNonSpecialPort2,
      utility::kCoppMidPriQueueId,
      srcPort);
}

} // namespace facebook::fboss::utility
