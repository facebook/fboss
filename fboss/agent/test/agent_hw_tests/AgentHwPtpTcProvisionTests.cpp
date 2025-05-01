/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/packet/PTPHeader.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"

#include <folly/IPAddress.h>

namespace {} // namespace

namespace facebook::fboss {

/*
 *  Test case to validate PTP TC on all ports.
 *  1. Select two ports, one ingress and one egress.
 *  2. Add Vlans and put ports in loopback mode (MAC), config egress port as
 * next hop.
 *  3. Inject PTP packet packet out of ingress port, packet received back on the
 * same port, RX timestamp applied. The packet is now forwarded out of egress
 * port, TX timestamp applied.
 *  4. Packet is received back on egress port, now forward the same to CPU,
 * capture the packet and check for CF field in the packet.
 *  5. Run for each port pairs.
 */
class AgentHwPtpTcProvisionTests : public AgentHwTest {
 protected:
  const std::string kdstIpPrefix = "2192::101:";
  const std::string knexthopMacPrefix = "aa:bb:cc:dd:ee:";
  const folly::IPAddressV6 kSrcIp = folly::IPAddressV6("2025::1");
  const folly::MacAddress kSrcMac = folly::MacAddress("aa:bb:cc:00:00:01");

  void SetUp() override {
    AgentHwTest::SetUp();
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::PTP_TC};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = checkSameAndGetAsic(l3Asics);
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    config.switchSettings()->ptpTcEnable() = true;
    auto ports = ensemble.masterLogicalInterfacePortIds();
    std::set<folly::CIDRNetwork> prefixs;
    for (int idx = 0; idx < ports.size(); idx++) {
      prefixs.emplace(getDstIp(idx), 128);
    }
    utility::addTrapPacketAcl(asic, &config, prefixs);
    return config;
  }

  int getPortIndexFromDstIp(folly::IPAddressV6 ip) {
    auto bytes = ip.toByteArray();
    return ((bytes[14] << 8) | bytes[15]) - 10;
  }

  folly::IPAddressV6 getDstIp(int portIndex) const {
    std::stringstream ss;
    auto offset = 10;
    ss << std::hex << portIndex + offset;
    return folly::IPAddressV6(kdstIpPrefix + ss.str());
  }

  int getPortIndexFromNexthopMax(folly::MacAddress mac) {
    auto lastByte = mac.bytes()[5];
    return lastByte - 10;
  }

  folly::MacAddress getNexthopMac(int portIndex) {
    std::stringstream ss;
    auto offset = 10;
    ss << std::hex << portIndex + offset;
    return folly::MacAddress(knexthopMacPrefix + ss.str());
  }

  std::string getPortName(const PortID& portId) const {
    for (const auto& portMap :
         std::as_const(*getSw()->getState()->getPorts())) {
      for (const auto& port : std::as_const(*portMap.second)) {
        if (port.second->getID() == portId) {
          return port.second->getName();
        }
      }
    }
    throw FbossError("No port with ID: ", portId);
  }

  void setupEcmpTraffic(
      const PortID& portID,
      const folly::MacAddress& nexthopMac,
      const folly::IPAddressV6& dstIp) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper6{
        getProgrammedState(), nexthopMac};
    auto dstPort = PortDescriptor(portID);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper6.resolveNextHops(in, {dstPort});
      return newState;
    });
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper6.programRoutes(
        &wrapper, {dstPort}, {RoutePrefix<folly::IPAddressV6>{dstIp, 128}});
  }

  void sendPtpPkts(
      PTPMessageType ptpType,
      const PortID& injectPortID,
      const folly::IPAddressV6& dstIp,
      const PortID& dstPort) {
    XLOGF(
        DBG2,
        "Sending PTP packet from port {} to port {}",
        getPortName(injectPortID),
        getPortName(dstPort));
    auto portDescriptor = PortDescriptor(injectPortID);
    auto intfID = getSw()->getState()->getInterfaceIDForPort(portDescriptor);
    auto vlanID =
        getSw()->getState()->getInterfaces()->getNodeIf(intfID)->getVlanID();
    auto matcher =
        getSw()->getScopeResolver()->scope(getSw()->getState(), portDescriptor);
    auto scope = getSw()->getScopeResolver()->scope(
        getSw()->getState()->getVlans()->getNode(vlanID));
    auto dstMac = utility::getInterfaceMac(getSw()->getState(), intfID);
    // Send out PTP packet
    auto ptpPkt = makePtpPkt(getVlanIDForTx().value(), dstIp, dstMac, ptpType);
    getAgentEnsemble()->sendPacketAsync(
        std::move(ptpPkt), portDescriptor, std::nullopt);
  }

  void verifyPtpPkts(
      PTPMessageType ptpType,
      utility::SwSwitchPacketSnooper& snooper,
      int& verifiedCount,
      std::vector<bool> ptpTcVerified,
      const std::vector<PortID>& ports) {
    auto pktBufOpt = snooper.waitForPacket(1);
    while (pktBufOpt.has_value()) {
      auto pktBuf = *pktBufOpt.value().get();
      folly::io::Cursor pktCursor(&pktBuf);
      if (!utility::isPtpEventPacket(pktCursor)) {
        // should not get non-PTP packet, if so, ignore and continue
        pktBufOpt = snooper.waitForPacket(1);
        continue;
      }
      PTPHeader ptpHdr(&pktCursor);

      pktCursor.reset(&pktBuf);
      EthHdr ethHdr(pktCursor);
      auto srcMac = ethHdr.getSrcMac();
      auto dstMac = ethHdr.getDstMac();
      if (srcMac == kSrcMac) {
        // packet trapped from inject port (No Rx timestamp yet), ignore and
        // continue
        pktBufOpt = snooper.waitForPacket(1);
        continue;
      }

      // use dstMac and dstIP to find the dst port and inject port
      auto dstPortIdx = getPortIndexFromNexthopMax(dstMac);
      IPv6Hdr ipHdr(pktCursor);
      auto dstIp = ipHdr.dstAddr;
      auto injectPortIdx = getPortIndexFromDstIp(dstIp);

      // Verify packet is received on the correct port
      EXPECT_EQ(dstPortIdx, (injectPortIdx + 1) % ports.size());

      // Verify PTP fields
      auto correctionField = ptpHdr.getCorrectionField();
      uint64_t cfInNsecs = (correctionField >> 16) & 0x0000ffffffffffff;
      XLOG(DBG2) << "PTP packet with CorrectionField (CF) set to " << cfInNsecs
                 << " for packet sent from "
                 << getPortName(ports[injectPortIdx])
                 << " and received on port " << getPortName(ports[dstPortIdx]);
      EXPECT_EQ(ptpType, ptpHdr.getPtpType());
      EXPECT_EQ(PTPVersion::PTP_V2, ptpHdr.getPtpVersion());
      EXPECT_EQ(PTP_DELAY_REQUEST_MSG_SIZE, ptpHdr.getPtpMessageLength());
      // Verify PTP correction field
      EXPECT_GT(cfInNsecs, 0);
      // Ideally we should have a upper bound threshold per ASIC.
      // For now, a generic value will be used as minor latency differences are
      // not a concern.
      EXPECT_LT(cfInNsecs, 8000);

      if (!ptpTcVerified[injectPortIdx]) {
        ptpTcVerified[injectPortIdx] = true;
        verifiedCount++;
      }

      // read next packet until buf empty
      pktBufOpt = snooper.waitForPacket(1);
    }
  }

  folly::MacAddress getIntfMac() const {
    return utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
  }

 private:
  std::unique_ptr<facebook::fboss::TxPacket> makePtpPkt(
      const VlanID& vlanID,
      const folly::IPAddressV6& dstIp,
      const folly::MacAddress& dstMac,
      PTPMessageType ptpType) {
    return utility::makePTPTxPacket(
        getSw(),
        vlanID,
        kSrcMac,
        dstMac,
        kSrcIp,
        dstIp,
        0 /* dscp */,
        5, /* Avoid ttl 1/0 punt*/
        ptpType);
  }
};

// Each port will be selected as Ingress port once and Egress port once.
TEST_F(AgentHwPtpTcProvisionTests, VerifyPtpTcDelayRequestOnPorts) {
  auto ports = masterLogicalPortIds();
  auto size = ports.size();
  std::vector<folly::MacAddress> nexthopMacs; // binding to ingress port
  std::vector<folly::IPAddressV6> dstIps; // binding to egress port
  for (int idx = 0; idx < size; idx++) {
    nexthopMacs.emplace_back(getNexthopMac(idx));
    dstIps.emplace_back(getDstIp(idx));
  }

  auto setup = [=, this]() {
    //  Add dst port as nhp
    for (int idx = 0; idx < size; idx++) {
      auto dstIdx = (idx + 1) % size;
      setupEcmpTraffic(ports[idx], nexthopMacs[dstIdx], dstIps[idx]);
    }
  };

  // Send packet using dstIps[ingressPortIdx]
  // The Eth header will be modified later to use nexthopMacs[egressPortIdx]
  // as dstMac once trapped from egress port
  auto verify = [=, this]() {
    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper-ptp");

    std::unique_ptr<std::thread> packetTxThread =
        std::make_unique<std::thread>([&]() {
          initThread("PtpPacketsTxThread");
          for (int idx = 0; idx < size; idx++) {
            auto dstIdx = (idx + 1) % size;
            sendPtpPkts(
                PTPMessageType::PTP_DELAY_REQUEST,
                ports[idx],
                dstIps[idx],
                ports[dstIdx]);
            usleep(100 * 1000); // sleep 100ms to avoid packet burst
          }
        });

    std::vector<bool> ptpTcVerified(size);
    auto verifiedCount = 0;

    checkWithRetry(
        [&]() {
          verifyPtpPkts(
              PTPMessageType::PTP_DELAY_REQUEST,
              snooper,
              verifiedCount,
              ptpTcVerified,
              ports);
          return verifiedCount == ports.size();
        },
        120,
        std::chrono::milliseconds(1000),
        " Verify PTP packets received on all ports");
    packetTxThread->join();
    packetTxThread.reset();
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
