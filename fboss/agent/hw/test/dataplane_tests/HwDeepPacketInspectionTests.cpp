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
  std::unique_ptr<TxPacket> makePacket(
      const folly::IPAddressV6& dstIp,
      std::optional<std::vector<uint8_t>> payload =
          std::optional<std::vector<uint8_t>>()) {
    folly::IPAddressV6 kSrcIp("1::1");
    const auto srcMac = utility::kLocalCpuMac();
    const auto dstMac = utility::kLocalCpuMac();
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        std::nullopt, // vlanID
        srcMac,
        dstMac,
        kSrcIp,
        dstIp,
        8000, // l4 src port
        8001, // l4 dst port
        0x24 << 2, // dscp
        255, // hopLimit
        payload);
    return txPacket;
  }
  int sendPacket(
      std::unique_ptr<TxPacket> txPacket,
      std::optional<PortID> frontPanelPort) {
    size_t txPacketSize = txPacket->buf()->length();

    XLOG(DBG5) << "\n"
               << folly::hexDump(
                      txPacket->buf()->data(), txPacket->buf()->length());

    if (frontPanelPort.has_value()) {
      getHwSwitch()->sendPacketOutOfPortAsync(
          std::move(txPacket), *frontPanelPort);
    } else {
      getHwSwitch()->sendPacketSwitchedAsync(std::move(txPacket));
    }
    return txPacketSize;
  }

 private:
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
};

TEST_F(HwDeepPacketInspectionTest, l3ForwardedPkt) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
  const auto kPort = ecmpHelper.ecmpPortDescriptorAt(0);
  auto setup = [this, kPort, &ecmpHelper]() {
    applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), {kPort}));
  };
  auto verify = [this, kPort, &ecmpHelper]() {
    auto ensemble = getHwSwitchEnsemble();
    auto entry = std::make_unique<HwTestPacketTrapEntry>(
        ensemble->getHwSwitch(), kPort.phyPortID());
    auto frontPanelPort = ecmpHelper.ecmpPortDescriptorAt(1).phyPortID();
    auto txPacket = makePacket(ecmpHelper.ip(kPort));
    auto nhopMac = ecmpHelper.nhop(0).mac;
    auto switchType = ensemble->getHwSwitch()->getSwitchType();
    auto ethFrame = switchType == cfg::SwitchType::VOQ
        ? utility::makeEthFrame(*txPacket, nhopMac)
        : utility::makeEthFrame(
              *txPacket,
              nhopMac,
              utility::getIngressVlan(getProgrammedState(), kPort.phyPortID()));

    auto snooper =
        std::make_unique<HwTestPacketSnooper>(ensemble, std::nullopt, ethFrame);
    sendPacket(std::move(txPacket), frontPanelPort);
    WITH_RETRIES({
      auto frameRx = snooper->waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
