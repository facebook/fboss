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

constexpr int kStartTtl = 3;
const folly::IPAddressV6 kIPv6Dst = folly::IPAddressV6("2::1"); // arbit
const auto kSrcMac = folly::MacAddress{"00:00:00:00:01:03"}; // arbit

class PtpTests : public LinkTest {
 public:
  std::unique_ptr<facebook::fboss::TxPacket> createPtpPkt(
      PTPMessageType ptpType) {
    // note: we are not creating flood here, but want routing
    // of packets so that TTL goes down from 255 -> 0
    auto vlan = utility::firstVlanID(sw()->getState());
    // TODO: Remove the dependency on VLAN below
    if (!vlan) {
      throw FbossError("VLAN id unavailable for test");
    }
    auto vlanId = *vlan;
    const auto dstMac = sw()->getPlatform()->getLocalMac();
    auto intf = sw()->getState()->getInterfaces()->getInterfaceInVlan(vlanId);

    auto srcIp = folly::IPAddressV6("1::1"); // arbit
    return utility::makePTPTxPacket(
        sw()->getHw(),
        vlanId,
        kSrcMac,
        dstMac,
        srcIp,
        kIPv6Dst,
        0 /* dscp */,
        kStartTtl,
        ptpType);
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
  programDefaultRoute(ecmpPorts, sw()->getPlatform()->getLocalMac());
  auto ptpType = PTPMessageType::PTP_DELAY_REQUEST;
  auto ptpPkt = createPtpPkt(ptpType);

  // Send out PTP packet
  sw()->getHw()->sendPacketOutOfPortSync(
      std::move(ptpPkt), (*ecmpPorts.begin()).phyPortID());

  bool validated = false;
  auto localMac = sw()->getPlatform()->getLocalMac();

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
    // Verify PTP fields unchanged.
    EXPECT_EQ(ptpType, ptpHdr.getPtpType());
    EXPECT_EQ(PTPVersion::PTP_V2, ptpHdr.getPtpVersion());
    EXPECT_EQ(PTP_DELAY_REQUEST_MSG_SIZE, ptpHdr.getPtpMessageLength());

    pktCursor.reset(&pktBuf);
    auto hopLimit = utility::getIpHopLimit(pktCursor);

    pktCursor.reset(&pktBuf);
    EthHdr ethHdr(pktCursor);
    auto srcMac = ethHdr.getSrcMac();
    auto dstMac = ethHdr.getDstMac();

    if (hopLimit == kStartTtl) {
      // this is the original pkt, and has no timestamp on it
      EXPECT_EQ(correctionField, 0);
      XLOG(DBG2)
          << "PTP packet found with CorrectionField (CF) set to 0 with hop limit : "
          << kStartTtl;

      // Original packet should have the same src and dst mac as we sent out
      EXPECT_EQ(srcMac, kSrcMac);
      EXPECT_EQ(dstMac, localMac);
    } else {
      EXPECT_GT(correctionField, 0);
      // nano secs is first 48-bits, last 16 bits is subnano secs (remove it)
      uint64_t cfInNsecs = (correctionField >> 16) & 0x0000ffffffffffff;
      XLOG(DBG2) << "PTP packet found with CorrectionField (CF) set "
                 << std::hex << cfInNsecs << ", ttl: " << hopLimit;
      // CF for first pkt is ~800nsecs for BCM and ~1.7 msecs for Tajo
      // Also account for loopback multiple times
      EXPECT_LT(cfInNsecs, 2000 * (kStartTtl - hopLimit));

      // Both src and mac address should be local mac
      EXPECT_EQ(srcMac, localMac);
      EXPECT_EQ(dstMac, localMac);

      validated = true;
      if (hopLimit == 1) {
        break;
      }
    }
  }

  EXPECT_TRUE(validated);
}
