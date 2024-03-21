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

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/LoadBalancerConfigApplier.h"
#include "fboss/agent/LoadBalancerUtils.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/lib/CommonUtils.h"

#include "folly/MacAddress.h"

#include <folly/gen/Base.h>
#include <gtest/gtest.h>
#include <sstream>

DEFINE_string(
    load_balance_traffic_src,
    "",
    "CSV file with source IP, port and destination IP, port for load balancing test. See P827101297 for example.");

namespace facebook::fboss::utility {
namespace {
std::vector<std::string> kTrafficFields = {"sip", "dip", "sport", "dport"};
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
size_t pumpRoCETraffic(
    bool isV6,
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic,
    int destPort,
    int hopLimit,
    std::optional<folly::MacAddress> srcMacAddr,
    int packetCount) {
  folly::MacAddress srcMac(
      srcMacAddr.has_value() ? *srcMacAddr
                             : MacAddressGenerator().get(dstMac.u64HBO() + 1));
  auto srcIp = folly::IPAddress(isV6 ? "1001::1" : "100.0.0.1");
  auto dstIp = folly::IPAddress(isV6 ? "2001::1" : "200.0.0.1");

  size_t txPacketSize = 0;
  XLOG(INFO) << "Send traffic with RoCE payload ..";
  for (auto i = 0; i < packetCount; ++i) {
    std::vector<uint8_t> rocePayload = {kUdfRoceOpcode, 0x40, 0xff, 0xff, 0x00};
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
        allocateFn,
        vlan,
        srcMac,
        dstMac,
        srcIp, /* fixed */
        dstIp, /* fixed */
        kRandomUdfL4SrcPort, /* arbit src port, fixed */
        destPort,
        0,
        hopLimit,
        rocePayload);
    txPacketSize = pkt->buf()->length();
    if (frontPanelPortToLoopTraffic) {
      sendFn(
          std::move(pkt), PortDescriptor(frontPanelPortToLoopTraffic.value()));
    } else {
      sendFn(std::move(pkt));
    }
  }
  return txPacketSize;
}

/*
 * The helper expects source file FLAGS_load_balance_traffic_src to be in CSV
 * format, where it should contain the following columns:
 *
 *   sip - source IP
 *   dip - destination IP
 *   sport - source port
 *   dport - destination port
 *
 *   Please see P827101297 for an example of how this file should be formatted.
 */
size_t pumpTrafficWithSourceFile(
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic,
    int hopLimit,
    std::optional<folly::MacAddress> srcMacAddr) {
  XLOG(DBG2) << "Using " << FLAGS_load_balance_traffic_src
             << " as source file for traffic generation";
  folly::MacAddress srcMac(
      srcMacAddr.has_value() ? *srcMacAddr
                             : MacAddressGenerator().get(dstMac.u64HBO() + 1));
  size_t pktSize = 0;
  // Use source file to generate traffic
  if (!FLAGS_load_balance_traffic_src.empty()) {
    std::ifstream srcFile(FLAGS_load_balance_traffic_src);
    std::string line;
    // Find index of SIP, DIP, SPort and DPort
    std::vector<int> indices;
    if (std::getline(srcFile, line)) {
      std::vector<std::string> parsedLine;
      folly::split(',', line, parsedLine);
      for (const auto& field : kTrafficFields) {
        for (int i = 0; i < parsedLine.size(); i++) {
          std::string column = parsedLine[i];
          column.erase(
              std::remove_if(
                  column.begin(),
                  column.end(),
                  [](auto const& c) -> bool { return !std::isalnum(c); }),
              column.end());
          if (field == column) {
            indices.push_back(i);
            break;
          }
        }
      }
      CHECK_EQ(kTrafficFields.size(), indices.size());
    } else {
      throw FbossError("Empty source file ", FLAGS_load_balance_traffic_src);
    }
    while (std::getline(srcFile, line)) {
      std::vector<std::string> parsedLine;
      folly::split(',', line, parsedLine);
      auto pkt = makeUDPTxPacket(
          allocateFn,
          vlan,
          srcMac,
          dstMac,
          folly::IPAddress(parsedLine[indices[0]]),
          folly::IPAddress(parsedLine[indices[1]]),
          folly::to<uint16_t>(parsedLine[indices[2]]),
          folly::to<uint16_t>(parsedLine[indices[3]]),
          0,
          hopLimit);
      pktSize = pkt->buf()->length();
      if (frontPanelPortToLoopTraffic) {
        sendFn(
            std::move(pkt),
            PortDescriptor(frontPanelPortToLoopTraffic.value()));
      } else {
        sendFn(std::move(pkt));
      }
    }
  } else {
    throw FbossError(
        "Using pumpTrafficWithSourceFile without source file specified");
  }
  return pktSize;
}

size_t pumpTraffic(
    bool isV6,
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
    folly::MacAddress dstMac,
    std::optional<VlanID> vlan,
    std::optional<PortID> frontPanelPortToLoopTraffic,
    int hopLimit,
    int numPackets,
    std::optional<folly::MacAddress> srcMacAddr) {
  if (!FLAGS_load_balance_traffic_src.empty()) {
    return pumpTrafficWithSourceFile(
        allocateFn,
        sendFn,
        dstMac,
        vlan,
        frontPanelPortToLoopTraffic,
        hopLimit,
        srcMacAddr);
  }
  size_t pktSize = 0;
  folly::MacAddress srcMac(
      srcMacAddr.has_value() ? *srcMacAddr
                             : MacAddressGenerator().get(dstMac.u64HBO() + 1));
  for (auto i = 0; i < int(std::sqrt(numPackets)); ++i) {
    auto srcIp = folly::IPAddress(folly::sformat(
        isV6 ? "1001::{}:{}" : "100.0.{}.{}", (i + 1) / 256, (i + 1) % 256));
    for (auto j = 0; j < int(numPackets / std::sqrt(numPackets)); ++j) {
      auto dstIp = folly::IPAddress(folly::sformat(
          isV6 ? "2001::{}:{}" : "201.0.{}.{}", (j + 1) / 256, (j + 1) % 256));
      auto pkt = makeUDPTxPacket(
          allocateFn,
          vlan,
          srcMac,
          dstMac,
          srcIp,
          dstIp,
          10000 + i,
          20000 + j,
          0,
          hopLimit);
      pktSize = pkt->buf()->length();
      if (frontPanelPortToLoopTraffic) {
        sendFn(
            std::move(pkt),
            PortDescriptor(frontPanelPortToLoopTraffic.value()));
      } else {
        sendFn(std::move(pkt));
      }
    }
  }
  return pktSize;
}

void pumpTraffic(
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
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
              allocateFn,
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
            sendFn(
                std::move(pkt),
                PortDescriptor(frontPanelPortToLoopTraffic.value()));
          } else {
            sendFn(std::move(pkt));
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
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
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
          allocateFn,
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
        sendFn(
            std::move(pkt),
            PortDescriptor(frontPanelPortToLoopTraffic.value()));
      } else {
        sendFn(std::move(pkt));
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
    AllocatePktFunc allocateFn,
    SendPktFunc sendFn,
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
        pkt = frame.getTxPacket(allocateFn);
      } else {
        pkt = frame.getTxPacket(allocateFn);
      }
      if (frontPanelPortToLoopTraffic) {
        sendFn(
            std::move(pkt),
            PortDescriptor(frontPanelPortToLoopTraffic.value()));
      } else {
        sendFn(std::move(pkt));
      }
    }
  }
}

void pumpTrafficAndVerifyLoadBalanced(
    std::function<void()> pumpTraffic,
    std::function<void()> clearPortStats,
    std::function<bool()> isLoadBalanced,
    bool loadBalanceExpected) {
  clearPortStats();
  pumpTraffic();
  if (loadBalanceExpected) {
    WITH_RETRIES(EXPECT_EVENTUALLY_TRUE(isLoadBalanced()));
  } else {
    EXPECT_FALSE(isLoadBalanced());
  }
}

bool isHwDeterministicSeed(
    HwSwitch* hwSwitch,
    const std::shared_ptr<SwitchState>& state,
    LoadBalancerID id) {
  if (!hwSwitch->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
    // hash seed is not programmed or configured
    return true;
  }
  auto lb = state->getLoadBalancers()->getNode(id);
  return lb->getSeed() == hwSwitch->generateDeterministicSeed(id);
}

void SendPktFunc::operator()(
    std::unique_ptr<TxPacket> pkt,
    std::optional<PortDescriptor> port,
    std::optional<uint8_t> queue) {
  func3_(std::move(pkt), std::move(port), queue);
}

AllocatePktFunc getAllocatePktFn(TestEnsembleIf* ensemble) {
  return [ensemble](uint32_t size) { return ensemble->allocatePacket(size); };
}

AllocatePktFunc getAllocatePktFn(SwSwitch* sw) {
  return [sw](uint32_t size) { return sw->allocatePacket(size); };
}

SendPktFunc getSendPktFunc(TestEnsembleIf* ensemble) {
  return SendPktFunc([ensemble](
                         std::unique_ptr<TxPacket> pkt,
                         std::optional<PortDescriptor> port,
                         std::optional<uint8_t> queue) {
    ensemble->sendPacketAsync(std::move(pkt), std::move(port), queue);
  });
}

SendPktFunc getSendPktFunc(SwSwitch* sw) {
  return SendPktFunc([sw](
                         std::unique_ptr<TxPacket> pkt,
                         std::optional<PortDescriptor> port,
                         std::optional<uint8_t> queue) {
    sw->sendPacketAsync(std::move(pkt), std::move(port), queue);
  });
}

} // namespace facebook::fboss::utility
