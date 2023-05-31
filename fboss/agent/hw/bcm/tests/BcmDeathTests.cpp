// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

extern "C" {
#include <bcm/l3.h>
}

namespace facebook::fboss {

class BcmDeathTest : public BcmTest {
 protected:
  void SetUp() override {
    BcmTest::SetUp();
    applyNewConfig(initialConfig());
  }

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports{
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        masterLogicalPortIds()[2],
    };
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        ports,
        {{cfg::PortType::INTERFACE_PORT, cfg::PortLoopbackMode::NONE}});
  }

  template <typename AddrT>
  void setupL3Route(
      ClientID client,
      AddrT network,
      uint8_t mask,
      boost::container::flat_set<AddrT> nexthops) {
    boost::container::flat_set<NextHop> nexthopSet;
    for (const auto& nexthop : nexthops) {
      nexthopSet.insert(UnresolvedNextHop(nexthop, ECMP_WEIGHT));
    }

    auto state = getProgrammedState();
    auto updater = getHwSwitchEnsemble()->getRouteUpdater();
    updater.addRoute(
        RouterID(0),
        network,
        mask,
        client,
        RouteNextHopEntry(
            std::move(nexthopSet), AdminDistance::MAX_ADMIN_DISTANCE));
    updater.program();
  }

  template <typename SetUpFn, typename DieFn>
  void verifyDeath(SetUpFn setup, DieFn die, const std::string& error) {
    ASSERT_DEATH(
        {
          setup();
          die();
        },
        error.c_str());
  }

  template <typename SetUpFn, typename DieFn>
  void verifyNoDeath(SetUpFn setup, DieFn die, const std::string& /*error*/) {
    setup();
    die();
  }
};

//

TEST_F(BcmDeathTest, DieOnHostRouteWithEcmpNextHops) {
  std::vector<NextHopWeight> weights{1};
  using PrefixV6 = typename Route<folly::IPAddressV6>::Prefix;
  std::array<PrefixV6, 3> prefixes{
      PrefixV6{folly::IPAddressV6("1::baac"), 127},
      PrefixV6{folly::IPAddressV6("2::baac"), 127},
      PrefixV6{folly::IPAddressV6("3::baac"), 127},
  };
  std::array<folly::IPAddressV6, 3> nexthops{
      folly::IPAddressV6("1::baad"),
      folly::IPAddressV6("2::baad"),
      folly::IPAddressV6("3::baad"),
  };

  PrefixV6 v6Prefix{folly::IPAddressV6("2401::dead:beef"), 128};

  boost::container::flat_set<folly::IPAddressV6> nextHopsA;
  boost::container::flat_set<folly::IPAddressV6> nextHopsB;
  auto helper = utility::EcmpSetupTargetedPorts<folly::IPAddressV6>(
      getProgrammedState(), RouterID(0));

  auto setup = [&]() {
    for (auto i = 0; i < prefixes.size(); i++) {
      if (i < prefixes.size()) {
        nextHopsB.insert(nexthops[i]);
      }
      if (i < (prefixes.size() - 1)) {
        nextHopsA.insert(nexthops[i]);
      }
      boost::container::flat_set<PortDescriptor> port;
      port.insert(helper.ecmpPortDescs(prefixes.size())[i]);
      applyNewState(
          helper.resolveNextHops(getProgrammedState(), std::move(port)));
    }
  };
  auto die = [&]() {
    ASSERT_NE(nextHopsA.size(), 0);
    ASSERT_NE(nextHopsB.size(), 0);
    setupL3Route(ClientID(0), v6Prefix.network(), v6Prefix.mask(), nextHopsA);
    setupL3Route(ClientID(0), v6Prefix.network(), v6Prefix.mask(), nextHopsB);
  };

  auto errorMsg =
      "failed to destroy L3 ECMP egress object .+ on unit 0: "
      "Operation still running, -10";
  verifyNoDeath(setup, die, errorMsg);
}

} // namespace facebook::fboss
