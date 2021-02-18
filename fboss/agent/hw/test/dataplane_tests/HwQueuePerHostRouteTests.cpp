/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestRouteUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include <string>

namespace facebook::fboss {

template <typename AddrT>
class HwQueuePerHostRouteTest : public HwLinkStateDependentTest {
 public:
  using Type = AddrT;

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC);
  }

  RouterID kRouterID() const {
    return RouterID(0);
  }

  cfg::AclLookupClass kLookupClass() const {
    return cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
  }

  int kQueueID() {
    return 2;
  }

  RoutePrefix<AddrT> kGetRoutePrefix() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return RoutePrefix<folly::IPAddressV4>{
          folly::IPAddressV4{"10.10.1.0"}, 24};
    } else {
      return RoutePrefix<folly::IPAddressV6>{
          folly::IPAddressV6{"2803:6080:d038:3063::"}, 64};
    }
  }

  void addRoutes(const std::vector<RoutePrefix<AddrT>>& routePrefixes) {
    auto kEcmpWidth = 1;
    utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper(
        getProgrammedState(), kRouterID());

    applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), kEcmpWidth));
    ecmpHelper.programRoutes(
        getRouteUpdateWrapper(), kEcmpWidth, routePrefixes);
  }

  std::shared_ptr<SwitchState> updateRoutesClassID(
      const std::shared_ptr<SwitchState>& inState,
      const std::map<RoutePrefix<AddrT>, std::optional<cfg::AclLookupClass>>&
          routePrefix2ClassID) {
    auto outState{inState->clone()};
    auto routeTable = outState->getRouteTables()->getRouteTable(kRouterID());
    auto routeTableRib =
        routeTable->template getRib<AddrT>()->modify(kRouterID(), &outState);

    for (const auto& [routePrefix, classID] : routePrefix2ClassID) {
      auto route = routeTableRib->routes()->getRouteIf(routePrefix);
      CHECK(route);
      route = route->clone();
      route->updateClassID(classID);
      routeTableRib->updateRoute(route);
    }
    return outState;
  }

  AddrT kSrcIP() {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return folly::IPAddressV4("1.0.0.1");
    } else {
      return folly::IPAddressV6("1::1");
    }
  }

  AddrT kDstIP() {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return folly::IPAddressV4("10.10.1.2");
    } else {
      return folly::IPAddressV6("2803:6080:d038:3063::2");
    }
  }

  void setupHelper() {
    auto newCfg{initialConfig()};
    utility::addQueuePerHostQueueConfig(&newCfg);
    utility::addQueuePerHostAcls(&newCfg);

    this->applyNewConfig(newCfg);
    this->addRoutes({this->kGetRoutePrefix()});

    auto state = this->updateRoutesClassID(
        this->getProgrammedState(),
        {{this->kGetRoutePrefix(), this->kLookupClass()}});
    this->applyNewState(state);
  }

  std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt() {
    auto vlanId =
        VlanID(*this->initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(this->getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        this->getHwSwitch(),
        vlanId,
        srcMac, // src mac
        intfMac, // dst mac
        this->kSrcIP(),
        this->kDstIP(),
        8000, // l4 src port
        8001 // l4 dst port
    );

    return txPacket;
  }

  void verifyHelper(bool useFrontPanel) {
    std::map<int, int64_t> beforeQueueOutPkts;
    for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
      beforeQueueOutPkts[queueId] =
          this->getLatestPortStats(this->masterLogicalPortIds()[0])
              .get_queueOutPackets_()
              .at(queueId);
    }

    auto txPacket = createUdpPkt();

    if (useFrontPanel) {
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), PortID(masterLogicalPortIds()[1]));
    } else {
      getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    }

    std::map<int, int64_t> afterQueueOutPkts;
    for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
      afterQueueOutPkts[queueId] =
          this->getLatestPortStats(this->masterLogicalPortIds()[0])
              .get_queueOutPackets_()
              .at(queueId);
    }

    /*
     *  Consider ACL with action to egress pkts through queue 2.
     *
     *  CPU originated packets:
     *     - Hits ACL (queue2Cnt = 1), egress through queue 2 of port0.
     *     - port0 is in loopback mode, so the packet gets looped back.
     *     - When packet is routed, its dstMAC gets overwritten. Thus, the
     *       looped back packet is not routed, and thus does not hit the ACL.
     *     - On some platforms, looped back packets for unknown MACs are
     *       flooded and counted on queue *before* the split horizon check
     *       (drop when srcPort == dstPort). This flooding always happens on
     *       queue 0, so expect one or more packets on queue 0.
     *
     *  Front panel packets (injected with pipeline bypass):
     *     - Egress out of port1 queue0 (pipeline bypass).
     *     - port1 is in loopback mode, so the packet gets looped back.
     *     - Rest of the workflow is same as above when CPU originated packet
     *       gets injected for switching.
     */
    for (auto [qid, beforePkts] : beforeQueueOutPkts) {
      auto pktsOnQueue = afterQueueOutPkts[qid] - beforePkts;

      XLOG(DBG0) << "queueId: " << qid << " pktsOnQueue: " << pktsOnQueue;

      if (qid == this->kQueueID()) {
        EXPECT_EQ(pktsOnQueue, 1);
      } else if (qid == 0) {
        EXPECT_GE(pktsOnQueue, 0);
      } else {
        EXPECT_EQ(pktsOnQueue, 0);
      }
    }
  }
};

using IpTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_SUITE(HwQueuePerHostRouteTest, IpTypes);

TYPED_TEST(HwQueuePerHostRouteTest, VerifyHostToQueueMappingClassIDCpu) {
  if (!this->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() {
    this->setupHelper();
    this->bringDownPort(this->masterLogicalPortIds()[1]);
  };

  auto verify = [=]() { this->verifyHelper(false /* cpu port */); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwQueuePerHostRouteTest, VerifyHostToQueueMappingClassIDFrontPanel) {
  if (!this->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() { this->setupHelper(); };

  auto verify = [=]() { this->verifyHelper(true /* front panel port */); };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
