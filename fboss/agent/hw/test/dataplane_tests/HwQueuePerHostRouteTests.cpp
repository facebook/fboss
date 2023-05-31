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
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
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
        getAsic()->desiredLoopbackModes());
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
    ecmpHelper.programRoutes(getRouteUpdater(), kEcmpWidth, routePrefixes);
  }

  void updateRoutesClassID(
      const std::map<RoutePrefix<AddrT>, std::optional<cfg::AclLookupClass>>&
          routePrefix2ClassID) {
    auto updater = getHwSwitchEnsemble()->getRouteUpdater();

    for (const auto& [routePrefix, classID] : routePrefix2ClassID) {
      updater.programClassID(
          kRouterID(),
          {{folly::IPAddress(routePrefix.network()), routePrefix.mask()}},
          classID,
          false /* sync*/);
    }
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

  void setupHelper(bool blockNeighbor) {
    auto newCfg{initialConfig()};
    utility::addQueuePerHostQueueConfig(&newCfg);
    utility::addQueuePerHostAcls(&newCfg);

    this->applyNewConfig(newCfg);
    this->addRoutes({this->kGetRoutePrefix()});

    this->updateRoutesClassID(
        {{this->kGetRoutePrefix(),
          blockNeighbor ? cfg::AclLookupClass::CLASS_DROP
                        : this->kLookupClass()}});
  }

  void verifyHelper(bool useFrontPanel, bool blockNeighbor) {
    auto vlanId = VlanID(*this->initialConfig().vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(this->getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    utility::verifyQueuePerHostMapping(
        getHwSwitch(),
        getHwSwitchEnsemble(),
        vlanId,
        srcMac,
        intfMac,
        this->kSrcIP(),
        this->kDstIP(),
        useFrontPanel,
        blockNeighbor,
        std::nullopt, /* l4SrcPort */
        std::nullopt, /* l4DstPort */
        std::nullopt); /* dscp */
  }
};

using IpTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_SUITE(HwQueuePerHostRouteTest, IpTypes);

TYPED_TEST(HwQueuePerHostRouteTest, VerifyHostToQueueMappingClassIDCpu) {
  if (!this->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() {
    this->setupHelper(false /* blockNeighbor */);
    this->bringDownPort(this->masterLogicalPortIds()[1]);
  };
  auto verify = [=]() {
    this->verifyHelper(false /* cpu port */, false /* block neighbor */);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwQueuePerHostRouteTest, VerifyHostToQueueMappingClassIDFrontPanel) {
  if (!this->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() { this->setupHelper(false /*blockNeighbor */); };
  auto verify = [=]() {
    this->verifyHelper(true /* front panel port */, false /* block neighbor */);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwQueuePerHostRouteTest, VerifyHostToQueueMappingClassIDCpuBlock) {
  if (!this->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() {
    this->setupHelper(true /* blockNeighbor */);
    this->bringDownPort(this->masterLogicalPortIds()[1]);
  };
  auto verify = [=]() {
    this->verifyHelper(false /* cpu port */, true /* block neighbor */);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(
    HwQueuePerHostRouteTest,
    VerifyHostToQueueMappingClassIDFrontPanelBlock) {
  if (!this->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [=]() { this->setupHelper(true /*blockNeighbor */); };
  auto verify = [=]() {
    this->verifyHelper(true /* front panel port */, true /*block neighbor */);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
