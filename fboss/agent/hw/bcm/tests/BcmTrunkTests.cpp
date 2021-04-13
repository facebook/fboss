/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TrunkUtils.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"

#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestTrunkUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/IPAddress.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <numeric>
#include <vector>

extern "C" {
#include <bcm/l3.h>
#include <bcm/pkt.h>
#include <bcm/port.h>
#include <bcm/trunk.h>
}

using facebook::fboss::bcmCheckError;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using namespace std::chrono_literals;

namespace facebook::fboss {

using utility::addAggPort;
using utility::enableTrunkPorts;

class BcmTrunkTest : public BcmLinkStateDependentTests {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC);
  }
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
  void packetReceived(RxPacket* pkt) noexcept override {
    numReceivedPackets_++;
    if (pkt->isFromAggregatePort()) {
      numReceivedTrunkPackets_++;
      recvAggPortID_ = pkt->getSrcAggregatePort();
    }
  }

  void applyConfigAndEnableTrunks(const cfg::SwitchConfig& switchCfg) {
    auto state = applyNewConfig(switchCfg);
    applyNewState(enableTrunkPorts(state));
  }

  int numReceivedPackets_{0};
  int numReceivedTrunkPackets_{0};
  AggregatePortID recvAggPortID_{0};
};

TEST_F(BcmTrunkTest, findTrunkApiChecks) {
  static const AggregatePortID aggID{42};
  std::vector<int> subPorts = {
      masterLogicalPortIds()[0], masterLogicalPortIds()[1]};

  auto setup = [=]() {
    auto cfg = initialConfig();
    addAggPort(aggID, subPorts, &cfg);
    applyConfigAndEnableTrunks(cfg);
  };
  auto verify = [=]() {
    auto trunkTable = getHwSwitch()->getTrunkTable();

    for (auto itr : subPorts) {
      EXPECT_EQ(
          BcmTrunk::findTrunk(getUnit(), static_cast<bcm_module_t>(0), itr)
              .value(),
          trunkTable->getBcmTrunkId(aggID));
    }
    EXPECT_EQ(
        BcmTrunk::findTrunk(
            getUnit(), static_cast<bcm_module_t>(0), masterLogicalPortIds()[2]),
        std::nullopt);
    for (auto itr : subPorts) {
      auto aggPortIdFromMap =
          trunkTable->portToAggPortGet(static_cast<PortID>(itr));
      EXPECT_NE(aggPortIdFromMap, std::nullopt);
      EXPECT_EQ(*aggPortIdFromMap, aggID);
    }
    EXPECT_EQ(
        trunkTable->portToAggPortGet(
            static_cast<PortID>(masterLogicalPortIds()[2])),
        std::nullopt);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmTrunkTest, packetSendReceiveSrcPort) {
  if (getAsic()->getAsicType() == HwAsic::AsicType::ASIC_TYPE_FAKE) {
    return;
  }
  static const AggregatePortID aggID{42};
  std::vector<int> subPorts = {
      masterLogicalPortIds()[0], masterLogicalPortIds()[1]};

  auto setup = [=]() {
    auto cfg = initialConfig();
    addAggPort(aggID, subPorts, &cfg);
    applyConfigAndEnableTrunks(cfg);
  };
  auto verify = [=]() {
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto payLoadSize = 256;
    int numPacketsToSend = 10;
    for (int i = 0; i < numPacketsToSend; i++) {
      auto txPacket = utility::makeEthTxPacket(
          getHwSwitch(),
          vlanId,
          srcMac,
          folly::MacAddress("01:80:c2:00:00:0e"),
          facebook::fboss::ETHERTYPE::ETHERTYPE_LLDP,
          std::vector<uint8_t>(payLoadSize, 0xff));
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), PortID(masterLogicalPortIds()[0]), std::nullopt);
    }
    auto retries = 10;
    while (numReceivedPackets_ != numPacketsToSend) {
      std::this_thread::sleep_for(20ms);
      retries--;
      if (!retries) {
        XLOG(ERR) << "Waiting for receving packets timed out after 200ms";
        break;
      }
    }
    EXPECT_EQ(numReceivedPackets_, numPacketsToSend);
    EXPECT_EQ(numReceivedTrunkPackets_, numPacketsToSend);
    EXPECT_EQ(recvAggPortID_, aggID);
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
