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
    case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
      throw FbossError(
          "AsicType ", hwAsic->getAsicType(), " doesn't support queue feature");
  }
  throw FbossError("Unexpected AsicType ", hwAsic->getAsicType());
}

cfg::ToCpuAction getCpuActionType(const HwAsic* hwAsic) {
  switch (hwAsic->getAsicType()) {
    case HwAsic::AsicType::ASIC_TYPE_FAKE:
    case HwAsic::AsicType::ASIC_TYPE_MOCK:
    case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
    case HwAsic::AsicType::ASIC_TYPE_TAJO:
      return cfg::ToCpuAction::COPY;
    case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
      throw FbossError(
          "AsicType ", hwAsic->getAsicType(), " doesn't support cpu action");
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

uint32_t getCoppQueuePps(const HwAsic* hwAsic, uint16_t queueId) {
  uint32_t pps;
  if (hwAsic->getAsicType() == HwAsic::AsicType::ASIC_TYPE_TAJO) {
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
  if (hwAsic->getAsicType() == HwAsic::AsicType::ASIC_TYPE_TAJO) {
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
  queue0.id_ref() = kCoppLowPriQueueId;
  queue0.name_ref() = "cpuQueue-low";
  queue0.streamType_ref() = getCpuDefaultStreamType(hwAsic);
  queue0.scheduling_ref() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight_ref() = kCoppLowPriWeight;
  if (setQueueRate) {
    queue0.portQueueRate_ref() = setPortQueueRate(hwAsic, kCoppLowPriQueueId);
  }
  if (!hwAsic->mmuQgroupsEnabled()) {
    queue0.reservedBytes_ref() = kCoppLowPriReservedBytes;
  }
  setPortQueueSharedBytes(queue0);
  cpuQueues.push_back(queue0);

  cfg::PortQueue queue1;
  queue1.id_ref() = kCoppDefaultPriQueueId;
  queue1.name_ref() = "cpuQueue-default";
  queue1.streamType_ref() = getCpuDefaultStreamType(hwAsic);
  queue1.scheduling_ref() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue1.weight_ref() = kCoppDefaultPriWeight;
  if (setQueueRate) {
    queue1.portQueueRate_ref() =
        setPortQueueRate(hwAsic, kCoppDefaultPriQueueId);
  }
  if (!hwAsic->mmuQgroupsEnabled()) {
    queue1.reservedBytes_ref() = kCoppDefaultPriReservedBytes;
  }
  setPortQueueSharedBytes(queue1);
  cpuQueues.push_back(queue1);

  cfg::PortQueue queue2;
  queue2.id_ref() = kCoppMidPriQueueId;
  queue2.name_ref() = "cpuQueue-mid";
  queue2.streamType_ref() = getCpuDefaultStreamType(hwAsic);
  queue2.scheduling_ref() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue2.weight_ref() = kCoppMidPriWeight;
  cpuQueues.push_back(queue2);

  cfg::PortQueue queue9;
  queue9.id_ref() = getCoppHighPriQueueId(hwAsic);
  queue9.name_ref() = "cpuQueue-high";
  queue9.streamType_ref() = getCpuDefaultStreamType(hwAsic);
  queue9.scheduling_ref() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue9.weight_ref() = kCoppHighPriWeight;
  cpuQueues.push_back(queue9);

  *config.cpuQueues_ref() = cpuQueues;
}

void setDefaultCpuTrafficPolicyConfig(
    cfg::SwitchConfig& config,
    const HwAsic* hwAsic) {
  auto cpuAcls = utility::defaultCpuAcls(hwAsic, config);
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

cfg::MatchAction createQueueMatchAction(
    int queueId,
    cfg::ToCpuAction toCpuAction) {
  cfg::MatchAction action;
  cfg::QueueMatchAction queueAction;
  queueAction.queueId_ref() = queueId;
  action.sendToQueue_ref() = queueAction;

  switch (toCpuAction) {
    case cfg::ToCpuAction::COPY:
      action.toCpuAction_ref() = cfg::ToCpuAction::COPY;
      break;
    case cfg::ToCpuAction::TRAP:
      action.toCpuAction_ref() = cfg::ToCpuAction::TRAP;
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
  acl.name_ref() =
      folly::to<std::string>("cpuPolicing-CPU-Port-Mcast-v6-", dstIp);

  acl.dstIp_ref() = dstIp;
  acl.srcPort_ref() = kCPUPort;
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
  acl.name_ref() = folly::to<std::string>(
      "cpuPolicing-high-", dstNetworkStr, "-network-control");
  acl.dstIp_ref() = dstNetworkStr;
  acl.dscp_ref() = 48;
  acls.push_back(
      std::make_pair(acl, createQueueMatchAction(highPriQueueId, toCpuAction)));
}

void addMidPriAclForNw(
    const folly::CIDRNetwork& dstNetwork,
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls) {
  cfg::AclEntry acl;
  auto dstIp = folly::to<std::string>(dstNetwork.first, "/", dstNetwork.second);
  acl.name_ref() = folly::to<std::string>("cpuPolicing-mid-", dstIp);
  acl.dstIp_ref() = dstIp;

  acls.push_back(std::make_pair(
      acl, createQueueMatchAction(utility::kCoppMidPriQueueId, toCpuAction)));
}

void sendTcpPkts(
    facebook::fboss::HwSwitch* hwSwitch,
    int numPktsToSend,
    VlanID vlanId,
    folly::MacAddress dstMac,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    PortID outPort,
    uint8_t trafficClass,
    std::optional<std::vector<uint8_t>> payload) {
  folly::MacAddress srcMac;

  if (!dstMac.isUnicast()) {
    // some arbitrary mac
    srcMac = folly::MacAddress("00:00:01:02:03:04");
  } else {
    srcMac = utility::MacAddressGenerator().get(dstMac.u64NBO() + 1);
  }

  // arbit
  const auto srcIp =
      folly::IPAddress(dstIpAddress.isV4() ? "1.1.1.2" : "1::10");
  for (int i = 0; i < numPktsToSend; i++) {
    auto txPacket = utility::makeTCPTxPacket(
        hwSwitch,
        vlanId,
        srcMac,
        dstMac,
        srcIp,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        dstIpAddress.isV4()
            ? trafficClass
            : trafficClass << 2, // v6 header takes entire TC byte with
                                 // trailing 2 bits for ECN. V4 header OTOH
                                 // expects only dscp value.
        255,
        payload);
    hwSwitch->sendPacketOutOfPortSync(std::move(txPacket), outPort);
  }
}

std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
    const HwSwitch* hwSwitch,
    VlanID vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIpAddress,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    uint8_t ttl) {
  auto txPacket = utility::makeUDPTxPacket(
      hwSwitch,
      vlanId,
      srcMac,
      dstMac,
      srcIpAddress,
      dstIpAddress,
      l4SrcPort,
      l4DstPort,
      48 << 2, // DSCP
      ttl);

  return txPacket;
}

uint64_t getQueueOutPacketsWithRetry(
    HwSwitch* hwSwitch,
    int queueId,
    int retryTimes,
    uint64_t expectedNumPkts) {
  uint64_t outPkts = 0, outBytes = 0;
  do {
    std::tie(outPkts, outBytes) =
        getCpuQueueOutPacketsAndBytes(hwSwitch, queueId);
    if (retryTimes == 0 || outPkts == expectedNumPkts) {
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

  return outPkts;
}

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId) {
  auto hwPortStats = getCpuQueueStats(hwSwitch);
  auto queueIter = hwPortStats.queueOutPackets__ref()->find(queueId);
  auto outPackets = (queueIter != hwPortStats.queueOutPackets__ref()->end())
      ? queueIter->second
      : 0;
  queueIter = hwPortStats.queueOutBytes__ref()->find(queueId);
  auto outBytes = (queueIter != hwPortStats.queueOutBytes__ref()->end())
      ? queueIter->second
      : 0;
  return std::pair(outPackets, outBytes);
}

std::pair<uint64_t, uint64_t> getCpuQueueOutDiscardPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId) {
  auto hwPortStats = getCpuQueueStats(hwSwitch);
  auto queueIter = hwPortStats.queueOutDiscardPackets__ref()->find(queueId);
  auto outDiscardPackets =
      (queueIter != hwPortStats.queueOutDiscardPackets__ref()->end())
      ? queueIter->second
      : 0;
  queueIter = hwPortStats.queueOutDiscardBytes__ref()->find(queueId);
  auto outDiscardBytes =
      (queueIter != hwPortStats.queueOutDiscardBytes__ref()->end())
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
  auto queueIter = hwPortStats.queueWatermarkBytes__ref()->find(queueId);
  return (
      (queueIter != hwPortStats.queueWatermarkBytes__ref()->end())
          ? queueIter->second
          : 0);
}

} // namespace facebook::fboss::utility
