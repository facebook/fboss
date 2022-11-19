// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/test/HwAgentTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/packet/PTPHeader.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonUtils.h"

#include "common/process/Process.h"

using namespace facebook::fboss;

constexpr int kStartTtl = 2;
const folly::IPAddressV6 kIPv6Dst = folly::IPAddressV6("2::1"); // arbit

class PtpTests : public LinkTest {
 public:
  void createPtpTraffic(
      const boost::container::flat_set<PortDescriptor>& ecmpPorts,
      PTPMessageType ptpPkt) {
    XLOG(DBG2) << "Create PTP traffic";
    programDefaultRoute(ecmpPorts, sw()->getPlatform()->getLocalMac());
    // pick the first port from the list
    auto outputPort = *ecmpPorts.begin();

    // note: we are not creating flood here, but want routing
    // of packets so that TTL goes down from 255 -> 0
    auto vlan = utility::firstVlanID(sw()->getState());
    // TODO: Remove the dependency on VLAN below
    if (!vlan) {
      throw FbossError("VLAN id unavailable for test");
    }
    auto vlanId = *vlan;
    const auto dstMac = sw()->getPlatform()->getLocalMac();
    const auto srcMac = folly::MacAddress{"00:00:00:00:01:03"}; // arbit

    auto intf = sw()->getState()->getInterfaces()->getInterfaceInVlan(vlanId);

    auto srcIp = folly::IPAddressV6("1::1"); // arbit
    auto txPacket = utility::makePTPTxPacket(
        sw()->getHw(),
        vlanId,
        srcMac,
        dstMac,
        srcIp,
        kIPv6Dst,
        0 /* dscp */,
        kStartTtl,
        ptpPkt);
    sw()->getHw()->sendPacketOutOfPortSync(
        std::move(txPacket), outputPort.phyPortID());
  }

 protected:
  void setCmdLineFlagOverrides() const override {
    // disable other processes which generate pkts
    FLAGS_enable_lldp = false;
    // tunnel interface enable especially on fboss2000 results
    FLAGS_tun_intf = false;
    LinkTest::setCmdLineFlagOverrides();
  }
};

// Purpose of this test is to validate Transparent clock (TC) for PTP
// TC embeds timestamps in CF field for PTP event messages such as
// Delay request, sync etc.
// This test enables PTP, send PTP delay request pkt from CPU and externally
// loopback to another port on the switch. (port B -> port A -> port B -> port A
// ..) But PTP doesn't generate timestamps on first hop, its because we
// originated pkt from CPU and  egress port can't detect meta data which it
// needs to timestamp this pkt.  On ingress, timestamp is updated in meta
// header. So subsequent loop of same pkt through the fwd pipeline generates
// timestamp on egress. All these pkts are punted to CPU when they ingress again
// on port A. So we send 1 pkt with TTL decrementing from 255 all the way to 0
// We capture these pkts coming to CPU.
// if (pkt == ptp {
//    if (pkt_ttl == 255) EXPECT_EQ(CF_field, 0)
//    else EXPECT_GT(CF_field, 0)
// }
TEST_F(PtpTests, verifyPtpTcDelayRequest) {
  auto ecmpPorts = getVlanOwningCabledPorts();
  // create ACL to trap any packets to CPU coming with given dst IP
  // Ideally we should have used the l4port (PTP_UDP_EVENT_PORT), but
  // SAI doesn't support this qualifier yet
  folly::CIDRNetwork dstPrefix = folly::CIDRNetwork{kIPv6Dst, 128};
  auto entry = HwTestPacketTrapEntry(sw()->getHw(), dstPrefix);
  HwAgentTestPacketSnooper snooper(sw()->getPacketObservers());
  createPtpTraffic(
      getVlanOwningCabledPorts(), PTPMessageType::PTP_DELAY_REQUEST);

  bool validated = false;
  while (true) {
    auto pktBufOpt = snooper.waitForPacket(10 /* seconds */);
    ASSERT_TRUE(pktBufOpt.has_value());

    auto pktBuf = *pktBufOpt.value().get();
    folly::io::Cursor pktCursor(&pktBuf);
    if (!utility::isPtpEventPacket(pktCursor)) {
      // if we continue to encounter non ptp packets
      // and hit threshold of kMaxPktLimit, abort the test
      // as something is wrong
      continue;
    }

    XLOG(DBG2) << "PTP event packet found";
    PTPHeader ptpHdr(&pktCursor);
    auto correctionField = ptpHdr.getCorrectionField();

    pktCursor.reset(&pktBuf);
    auto hopLimit = utility::getIpHopLimit(pktCursor);

    if (hopLimit == kStartTtl) {
      // this is the original pkt, and has no timestamp on it
      EXPECT_EQ(correctionField, 0);
      XLOG(DBG2)
          << "PTP packet found with CorrectionField (CF) set to 0 with hop limit : "
          << kStartTtl;
    } else {
      EXPECT_GT(correctionField, 0);
      // nano secs is first 48-bits, last 16 bits is subnano secs (remove it)
      uint64_t cfInNsecs = (correctionField >> 16) & 0x0000ffffffffffff;
      XLOG(DBG2) << "PTP packet found with CorrectionField (CF) set "
                 << std::hex << cfInNsecs << ", ttl: " << hopLimit;
      // CF for first pkt is ~800nsecs for BCM and ~1.7 msecs for Tajo
      EXPECT_LT(cfInNsecs, 2000);
      validated = true;
      break;
    }
  }

  EXPECT_TRUE(validated);
}
