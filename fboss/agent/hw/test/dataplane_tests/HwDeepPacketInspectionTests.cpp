// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/CommonUtils.h"

namespace {
static folly::IPAddressV6 kSrcIp() {
  return folly::IPAddressV6("1::1");
}
} // namespace
namespace facebook::fboss {
class HwDeepPacketInspectionTest : public HwLinkStateDependentTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        true /*interfaceHasSubnet*/);
    utility::addOlympicQosMaps(config, getAsic());
    return config;
  }

 protected:
  utility::EcmpSetupAnyNPorts6 ecmpHelper() const {
    return utility::EcmpSetupAnyNPorts6(getProgrammedState());
  }
  PortDescriptor kPort() const {
    return ecmpHelper().ecmpPortDescriptorAt(0);
  }
  std::unique_ptr<TxPacket> makeUdpPacket(
      const folly::IPAddressV6& dstIp,
      std::optional<std::vector<uint8_t>> payload =
          std::optional<std::vector<uint8_t>>()) {
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        std::nullopt, // vlanID
        utility::kLocalCpuMac(),
        utility::kLocalCpuMac(),
        kSrcIp(),
        dstIp,
        8000, // l4 src port
        8001, // l4 dst port
        0x24 << 2, // dscp
        255, // hopLimit
        payload);
    return txPacket;
  }
  void sendPacketAndVerify(
      std::unique_ptr<TxPacket> txPacket,
      std::optional<PortID> frontPanelPort) {
    XLOG(DBG5) << "\n"
               << folly::hexDump(
                      txPacket->buf()->data(), txPacket->buf()->length());
    HwTestPacketTrapEntry trapEntry(getHwSwitch(), kPort().phyPortID());
    auto nhopMac = ecmpHelper().nhop(0).mac;
    auto switchType = getHwSwitch()->getSwitchType();

    auto ethFrame = switchType == cfg::SwitchType::VOQ
        ? utility::makeEthFrame(*txPacket, nhopMac)
        : utility::makeEthFrame(
              *txPacket,
              nhopMac,
              utility::getIngressVlan(
                  getProgrammedState(), kPort().phyPortID()));

    HwTestPacketSnooper snooper(getHwSwitchEnsemble(), std::nullopt, ethFrame);
    if (frontPanelPort.has_value()) {
      getHwSwitch()->sendPacketOutOfPortAsync(
          std::move(txPacket), *frontPanelPort);
    } else {
      getHwSwitch()->sendPacketSwitchedAsync(std::move(txPacket));
    }
    WITH_RETRIES({
      auto frameRx = snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });
  }

 private:
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
};

TEST_F(HwDeepPacketInspectionTest, l3ForwardedPkt) {
  auto setup = [this]() {
    applyNewState(
        ecmpHelper().resolveNextHops(getProgrammedState(), {kPort()}));
  };
  auto verify = [this]() {
    auto frontPanelPort = ecmpHelper().ecmpPortDescriptorAt(1).phyPortID();
    auto txPacket = makeUdpPacket(ecmpHelper().ip(kPort()));
    sendPacketAndVerify(std::move(txPacket), frontPanelPort);
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
