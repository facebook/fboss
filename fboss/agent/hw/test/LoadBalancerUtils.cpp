/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/LoadBalancerUtils.h"

#include <folly/IPAddress.h>

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/LoadBalancerConfigApplier.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "folly/MacAddress.h"

#include <folly/gen/Base.h>
#include <sstream>

namespace facebook::fboss::utility {
namespace {
cfg::Fields getHalfHashFields() {
  cfg::Fields hashFields;
  hashFields.ipv4Fields() = std::set<cfg::IPv4Field>(
      {cfg::IPv4Field::SOURCE_ADDRESS, cfg::IPv4Field::DESTINATION_ADDRESS});
  hashFields.ipv6Fields() = std::set<cfg::IPv6Field>(
      {cfg::IPv6Field::SOURCE_ADDRESS, cfg::IPv6Field::DESTINATION_ADDRESS});

  return hashFields;
}

cfg::Fields getFullHashFields() {
  auto hashFields = getHalfHashFields();
  hashFields.transportFields() = std::set<cfg::TransportField>(
      {cfg::TransportField::SOURCE_PORT,
       cfg::TransportField::DESTINATION_PORT});
  return hashFields;
}

cfg::Fields getFullHashUdf() {
  auto hashFields = getHalfHashFields();
  hashFields.transportFields() = std::set<cfg::TransportField>(
      {cfg::TransportField::SOURCE_PORT,
       cfg::TransportField::DESTINATION_PORT});
  hashFields.udfGroups() = std::vector<std::string>({"dstQueuePair"});
  return hashFields;
}

cfg::LoadBalancer getHalfHashConfig(
    const Platform* platform,
    cfg::LoadBalancerID id) {
  cfg::LoadBalancer loadBalancer;
  *loadBalancer.id() = id;
  if (platform->getAsic()->isSupported(
          HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    *loadBalancer.fieldSelection() = getHalfHashFields();
  }
  *loadBalancer.algorithm() = cfg::HashingAlgorithm::CRC16_CCITT;
  return loadBalancer;
}
cfg::LoadBalancer getFullHashConfig(
    const Platform* platform,
    cfg::LoadBalancerID id) {
  cfg::LoadBalancer loadBalancer;
  *loadBalancer.id() = id;
  if (platform->getAsic()->isSupported(
          HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    *loadBalancer.fieldSelection() = getFullHashFields();
  }
  *loadBalancer.algorithm() = cfg::HashingAlgorithm::CRC16_CCITT;
  return loadBalancer;
}
cfg::LoadBalancer getFullHashUdfConfig(
    const Platform* platform,
    cfg::LoadBalancerID id) {
  cfg::LoadBalancer loadBalancer;
  *loadBalancer.id() = id;
  if (platform->getAsic()->isSupported(
          HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    *loadBalancer.fieldSelection() = getFullHashUdf();
  }
  *loadBalancer.algorithm() = cfg::HashingAlgorithm::CRC16_CCITT;
  return loadBalancer;
}
cfg::LoadBalancer getTrunkHalfHashConfig(const Platform* platform) {
  return getHalfHashConfig(platform, cfg::LoadBalancerID::AGGREGATE_PORT);
}
cfg::LoadBalancer getTrunkFullHashConfig(const Platform* platform) {
  return getFullHashConfig(platform, cfg::LoadBalancerID::AGGREGATE_PORT);
}
} // namespace
cfg::LoadBalancer getEcmpHalfHashConfig(const Platform* platform) {
  return getHalfHashConfig(platform, cfg::LoadBalancerID::ECMP);
}
cfg::LoadBalancer getEcmpFullHashConfig(const Platform* platform) {
  return getFullHashConfig(platform, cfg::LoadBalancerID::ECMP);
}
cfg::LoadBalancer getEcmpFullUdfHashConfig(const Platform* platform) {
  return getFullHashUdfConfig(platform, cfg::LoadBalancerID::ECMP);
}
std::vector<cfg::LoadBalancer> getEcmpFullTrunkHalfHashConfig(
    const Platform* platform) {
  return {getEcmpFullHashConfig(platform), getTrunkHalfHashConfig(platform)};
}
std::vector<cfg::LoadBalancer> getEcmpHalfTrunkFullHashConfig(
    const Platform* platform) {
  return {getEcmpHalfHashConfig(platform), getTrunkFullHashConfig(platform)};
}
std::vector<cfg::LoadBalancer> getEcmpFullTrunkFullHashConfig(
    const Platform* platform) {
  return {getEcmpFullHashConfig(platform), getTrunkFullHashConfig(platform)};
}
std::shared_ptr<SwitchState> setLoadBalancer(
    const Platform* platform,
    const std::shared_ptr<SwitchState>& inputState,
    const cfg::LoadBalancer& loadBalancerCfg) {
  return addLoadBalancers(platform, inputState, {loadBalancerCfg});
}

std::shared_ptr<SwitchState> addLoadBalancers(
    const Platform* platform,
    const std::shared_ptr<SwitchState>& inputState,
    const std::vector<cfg::LoadBalancer>& loadBalancerCfgs) {
  if (!platform->getAsic()->isSupported(
          HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    // configuring hash is not supported.
    XLOG(WARNING) << "load balancer configuration is not supported.";
    return inputState;
  }
  auto newState{inputState->clone()};
  auto lbMap = newState->getLoadBalancers()->clone();
  for (const auto& loadBalancerCfg : loadBalancerCfgs) {
    auto loadBalancer =
        LoadBalancerConfigParser(platform).parse(loadBalancerCfg);
    if (lbMap->getLoadBalancerIf(loadBalancer->getID())) {
      lbMap->updateLoadBalancer(loadBalancer);
    } else {
      lbMap->addLoadBalancer(loadBalancer);
    }
  }
  newState->resetLoadBalancers(lbMap);
  return newState;
}

cfg::UdfConfig addUdfConfig(void) {
  cfg::UdfConfig udfCfg;
  cfg::UdfGroup udfGroupEntry;
  cfg::UdfPacketMatcher matchCfg;
  std::map<std::string, cfg::UdfGroup> udfMap;
  std::map<std::string, cfg::UdfPacketMatcher> udfPacketMatcherMap;

  matchCfg.name() = "l4UdpRoce";
  matchCfg.l4PktType() = cfg::UdfMatchL4Type::UDF_L4_PKT_TYPE_UDP;
  matchCfg.UdfL4DstPort() = 4091;

  udfGroupEntry.name() = "dstQueuePair";
  udfGroupEntry.header() = cfg::UdfBaseHeaderType::UDF_L4_HEADER;
  udfGroupEntry.startOffsetInBytes() = 5;
  udfGroupEntry.fieldSizeInBytes() = 3;
  // has to be the same as in matchCfg
  udfGroupEntry.udfPacketMatcherIds() = {"l4UdpRoce"};

  udfMap.insert(std::make_pair(*udfGroupEntry.name(), udfGroupEntry));
  udfPacketMatcherMap.insert(std::make_pair(*matchCfg.name(), matchCfg));
  udfCfg.udfGroups() = udfMap;
  udfCfg.udfPacketMatcher() = udfPacketMatcherMap;
  return udfCfg;
}

/*
 *  RoCEv2 header lookss as below
 *  Dst Queue Pair field is in the middle
 *  of the header. (below fields in bits)
 *   ----------------------------------
 *  | Opcode(8)  | Flags(8) | Key (16) |
 *  --------------- -------------------
 *  | Resvd (8)  | Dst Queue Pair (24) |
 *  -----------------------------------
 *  | Ack Req(8) | Packet Seq num (24) |
 *  ------------------------------------
 */
void pumpRoCETraffic(
    bool isV6,
    HwSwitch* hw,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic,
    int hopLimit,
    std::optional<folly::MacAddress> srcMacAddr) {
  folly::MacAddress srcMac(
      srcMacAddr.has_value() ? *srcMacAddr
                             : MacAddressGenerator().get(dstMac.u64HBO() + 1));
  auto srcIp = folly::IPAddress(isV6 ? "1001::1" : "100.0.0.1");
  auto dstIp = folly::IPAddress(isV6 ? "2001::1" : "200.0.0.1");

  XLOG(INFO) << "Send traffic with RoCE payload ..";
  for (auto i = 0; i < 10000; ++i) {
    std::vector<uint8_t> rocePayload = {0x0a, 0x40, 0xff, 0xff, 0x00};
    std::vector<uint8_t> roceEndPayload = {0x40, 0x00, 0x00, 0x03};

    // vary dst queues pair ids ONLY in the RoCE pkt
    // to verify that we can hash on it
    int dstQueueIds = i;
    // since dst queue pair id is in the middle of the packet
    // we need to keep front/end payload which doesn't vary
    rocePayload.push_back((dstQueueIds & 0x00ff0000) >> 16);
    rocePayload.push_back((dstQueueIds & 0x0000ff00) >> 8);
    rocePayload.push_back((dstQueueIds & 0x000000ff));
    // 12 byte payload
    std::move(
        roceEndPayload.begin(),
        roceEndPayload.end(),
        std::back_inserter(rocePayload));
    auto pkt = makeUDPTxPacket(
        hw,
        vlan,
        srcMac,
        dstMac,
        srcIp, /* fixed */
        dstIp, /* fixed */
        62946, /* arbit src port, fixed */
        4791, /* RoCE fixed dst port */
        0,
        hopLimit,
        rocePayload);
    if (frontPanelPortToLoopTraffic) {
      hw->sendPacketOutOfPortSync(
          std::move(pkt), frontPanelPortToLoopTraffic.value());
    } else {
      hw->sendPacketSwitchedSync(std::move(pkt));
    }
  }
}

void pumpTraffic(
    bool isV6,
    HwSwitch* hw,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic,
    int hopLimit,
    std::optional<folly::MacAddress> srcMacAddr) {
  folly::MacAddress srcMac(
      srcMacAddr.has_value() ? *srcMacAddr
                             : MacAddressGenerator().get(dstMac.u64HBO() + 1));
  for (auto i = 0; i < 100; ++i) {
    auto srcIp = folly::IPAddress(
        folly::sformat(isV6 ? "1001::{}" : "100.0.0.{}", i + 1));
    for (auto j = 0; j < 100; ++j) {
      auto dstIp = folly::IPAddress(
          folly::sformat(isV6 ? "2001::{}" : "200.0.0.{}", j + 1));
      auto pkt = makeUDPTxPacket(
          hw,
          vlan,
          srcMac,
          dstMac,
          srcIp,
          dstIp,
          10000 + i,
          20000 + j,
          0,
          hopLimit);
      if (frontPanelPortToLoopTraffic) {
        hw->sendPacketOutOfPortSync(
            std::move(pkt), frontPanelPortToLoopTraffic.value());
      } else {
        hw->sendPacketSwitchedSync(std::move(pkt));
      }
    }
  }
}

void pumpTraffic(
    HwSwitch* hw,
    folly::MacAddress dstMac,
    std::vector<folly::IPAddress> srcIps,
    std::vector<folly::IPAddress> dstIps,
    uint16_t srcPort,
    uint16_t dstPort,
    uint8_t streams,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic,
    int hopLimit,
    std::optional<folly::MacAddress> srcMacAddr,
    int numPkts) {
  folly::MacAddress srcMac(
      srcMacAddr.has_value() ? *srcMacAddr
                             : MacAddressGenerator().get(dstMac.u64HBO() + 1));
  for (auto srcIp : srcIps) {
    for (auto dstIp : dstIps) {
      CHECK_EQ(srcIp.isV4(), dstIp.isV4());
      for (auto i = 0; i < streams; i++) {
        while (numPkts--) {
          auto pkt = makeUDPTxPacket(
              hw,
              vlan,
              srcMac,
              dstMac,
              srcIp,
              dstIp,
              srcPort + i,
              dstPort + i,
              0,
              hopLimit);
          if (frontPanelPortToLoopTraffic) {
            hw->sendPacketOutOfPortSync(
                std::move(pkt), frontPanelPortToLoopTraffic.value());
          } else {
            hw->sendPacketSwitchedSync(std::move(pkt));
          }
        }
      }
    }
  }
}
/*
 * Generate traffic with random source ip, destination ip, source port and
 * destination port. every run will pump same random traffic as random number
 * generator is seeded with constant value. in an attempt to unify hash
 * configurations across switches in network, full hash is considered to be
 * present on all switches. this causes polarization in tests and vendor
 * recommends not to use traffic where source and destination fields (ip and
 * port) are only incremented by 1 but to use somewhat random traffic. however
 * random traffic should be deterministic. this function attempts to provide the
 * deterministic random traffic for experimentation and use in the load balancer
 * tests.
 */
void pumpDeterministicRandomTraffic(
    bool isV6,
    HwSwitch* hw,
    folly::MacAddress intfMac,
    VlanID vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic,
    int hopLimit) {
  static uint32_t count = 0;
  uint32_t counter = 1;

  RandomNumberGenerator srcV4(0, 0, 0xFF);
  RandomNumberGenerator srcV6(0, 0, 0xFFFF);
  RandomNumberGenerator dstV4(1, 0, 0xFF);
  RandomNumberGenerator dstV6(1, 0, 0xFFFF);
  RandomNumberGenerator srcPort(2, 10001, 10100);
  RandomNumberGenerator dstPort(2, 20001, 20100);

  auto intToHex = [](auto i) {
    std::stringstream stream;
    stream << std::hex << i;
    return stream.str();
  };

  auto srcMac = MacAddressGenerator().get(intfMac.u64HBO() + 1);
  for (auto i = 0; i < 1000; ++i) {
    auto srcIp = isV6
        ? folly::IPAddress(folly::sformat("1001::{}", intToHex(srcV6())))
        : folly::IPAddress(folly::sformat("100.0.0.{}", srcV4()));
    for (auto j = 0; j < 100; ++j) {
      auto dstIp = isV6
          ? folly::IPAddress(folly::sformat("2001::{}", intToHex(dstV6())))
          : folly::IPAddress(folly::sformat("200.0.0.{}", dstV4()));

      auto pkt = makeUDPTxPacket(
          hw,
          vlan,
          srcMac,
          intfMac,
          srcIp,
          dstIp,
          srcPort(),
          dstPort(),
          0,
          hopLimit);
      if (frontPanelPortToLoopTraffic) {
        hw->sendPacketOutOfPortSync(
            std::move(pkt), frontPanelPortToLoopTraffic.value());
      } else {
        hw->sendPacketSwitchedSync(std::move(pkt));
      }
      count++;
      if (count % 1000 == 0) {
        XLOG(DBG2) << counter << " . sent " << count << " packets";
        counter++;
      }
    }
  }
  XLOG(DBG2) << "Sent total of " << count << " packets";
}

void pumpMplsTraffic(
    bool isV6,
    HwSwitch* hw,
    uint32_t label,
    folly::MacAddress intfMac,
    VlanID vlanId,
    std::optional<PortID> frontPanelPortToLoopTraffic) {
  MPLSHdr::Label mplsLabel{label, 0, true, 128};
  std::unique_ptr<TxPacket> pkt;
  for (auto i = 0; i < 100; ++i) {
    auto srcIp = folly::IPAddress(
        folly::sformat(isV6 ? "1001::{}" : "100.0.0.{}", i + 1));
    for (auto j = 0; j < 100; ++j) {
      auto dstIp = folly::IPAddress(
          folly::sformat(isV6 ? "2001::{}" : "200.0.0.{}", j + 1));

      auto frame = isV6 ? utility::getEthFrame(
                              intfMac,
                              intfMac,
                              {mplsLabel},
                              srcIp.asV6(),
                              dstIp.asV6(),
                              10000 + i,
                              20000 + j,
                              vlanId)
                        : utility::getEthFrame(
                              intfMac,
                              intfMac,
                              {mplsLabel},
                              srcIp.asV4(),
                              dstIp.asV4(),
                              10000 + i,
                              20000 + j,
                              vlanId);

      if (isV6) {
        pkt = frame.getTxPacket(hw);
      } else {
        pkt = frame.getTxPacket(hw);
      }

      if (frontPanelPortToLoopTraffic) {
        hw->sendPacketOutOfPortSync(
            std::move(pkt), frontPanelPortToLoopTraffic.value());
      } else {
        hw->sendPacketSwitchedSync(std::move(pkt));
      }
    }
  }
}

template <typename IdT>
bool isLoadBalancedImpl(
    const std::map<IdT, HwPortStats>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk) {
  auto ecmpPorts = folly::gen::from(portIdToStats) |
      folly::gen::map([](const auto& portIdAndStats) {
                     return portIdAndStats.first;
                   }) |
      folly::gen::as<std::vector<IdT>>();

  auto portBytes = folly::gen::from(portIdToStats) |
      folly::gen::map([](const auto& portIdAndStats) {
                     return *portIdAndStats.second.outBytes_();
                   }) |
      folly::gen::as<std::set<uint64_t>>();

  auto lowest = *portBytes.begin();
  auto highest = *portBytes.rbegin();
  XLOG(DBG0) << " Highest bytes: " << highest << " lowest bytes: " << lowest;
  if (!lowest) {
    return !highest && noTrafficOk;
  }
  if (!weights.empty()) {
    auto maxWeight = *(std::max_element(weights.begin(), weights.end()));
    for (auto i = 0; i < portIdToStats.size(); ++i) {
      auto portOutBytes = *portIdToStats.find(ecmpPorts[i])->second.outBytes_();
      auto weightPercent = (static_cast<float>(weights[i]) / maxWeight) * 100.0;
      auto portOutBytesPercent =
          (static_cast<float>(portOutBytes) / highest) * 100.0;
      auto percentDev = std::abs(weightPercent - portOutBytesPercent);
      // Don't tolerate a deviation of more than maxDeviationPct
      XLOG(DBG2) << "Percent Deviation: " << percentDev
                 << ", Maximum Deviation: " << maxDeviationPct;
      if (percentDev > maxDeviationPct) {
        return false;
      }
    }
  } else {
    auto percentDev = (static_cast<float>(highest - lowest) / lowest) * 100.0;
    // Don't tolerate a deviation of more than maxDeviationPct
    XLOG(DBG2) << "Percent Deviation: " << percentDev
               << ", Maximum Deviation: " << maxDeviationPct;
    if (percentDev > maxDeviationPct) {
      return false;
    }
  }
  return true;
}

bool isLoadBalanced(
    const std::map<PortID, HwPortStats>& portStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk) {
  return isLoadBalancedImpl(portStats, weights, maxDeviationPct, noTrafficOk);
}
bool isLoadBalanced(
    const std::map<PortID, HwPortStats>& portStats,
    int maxDeviationPct) {
  return isLoadBalanced(
      portStats, std::vector<NextHopWeight>(), maxDeviationPct);
}

bool isLoadBalanced(
    const std::map<std::string, HwPortStats>& portStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk) {
  return isLoadBalancedImpl(portStats, weights, maxDeviationPct, noTrafficOk);
}

bool isLoadBalanced(
    const std::map<std::string, HwPortStats>& portStats,
    int maxDeviationPct) {
  return isLoadBalanced(
      portStats, std::vector<NextHopWeight>(), maxDeviationPct);
}

bool isLoadBalanced(
    const std::vector<PortDescriptor>& ecmpPorts,
    const std::vector<NextHopWeight>& weights,
    std::function<std::map<PortID, HwPortStats>(const std::vector<PortID>&)>
        getPortStatsFn,
    int maxDeviationPct,
    bool noTrafficOk) {
  auto portIDs = folly::gen::from(ecmpPorts) |
      folly::gen::map([](const auto& portDesc) {
                   CHECK(portDesc.isPhysicalPort());
                   return portDesc.phyPortID();
                 }) |
      folly::gen::as<std::vector<PortID>>();
  auto portIdToStats = getPortStatsFn(portIDs);
  return isLoadBalanced(portIdToStats, weights, maxDeviationPct, noTrafficOk);
}

bool isLoadBalanced(
    HwSwitchEnsemble* hwSwitchEnsemble,
    const std::vector<PortDescriptor>& ecmpPorts,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk) {
  auto getPortStatsFn =
      [&](const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
    return hwSwitchEnsemble->getLatestPortStats(portIds);
  };
  return isLoadBalanced(
      ecmpPorts, weights, getPortStatsFn, maxDeviationPct, noTrafficOk);
}

bool isLoadBalanced(
    HwSwitchEnsemble* hwSwitchEnsemble,
    const std::vector<PortDescriptor>& ecmpPorts,
    int maxDeviationPct) {
  return isLoadBalanced(
      hwSwitchEnsemble,
      ecmpPorts,
      std::vector<NextHopWeight>(),
      maxDeviationPct);
}

bool pumpTrafficAndVerifyLoadBalanced(
    std::function<void()> pumpTraffic,
    std::function<void()> clearPortStats,
    std::function<bool()> isLoadBalanced,
    int retries) {
  bool loadBalanced = false;
  for (auto i = 0; i < retries && !loadBalanced; i++) {
    clearPortStats();
    pumpTraffic();
    loadBalanced = isLoadBalanced();
  }
  return loadBalanced;
}

} // namespace facebook::fboss::utility
