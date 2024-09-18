/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/ProdConfigFactory.h"

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTestProdConfigUtils.h"

#include "fboss/agent/hw/test/dataplane_tests/HwProdInvariantHelper.h"

#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestPfcUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

#include "fboss/agent/state/Port.h"

DEFINE_bool(
    dynamic_config,
    false,
    "enable to load HwProdInvariants config from --config flag");

namespace facebook::fboss {
/*
 * Test to verify that standard invariants hold. DO NOT change the name
 * of this test - its used in testing prod<->trunk warmboots on diff
 */
class HwProdInvariantsTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    if (!FLAGS_dynamic_config) {
      return initConfigHelper();
    }

    // Dynamic config testing is enabled with the --dynamic_config flag, so make
    // sure if it's enabled that there's something in the --config arg.
    // As of right now, --config defaults to "/etc/coop/agent.conf", so this
    // should never be a problem... but you never know.
    if (FLAGS_config.empty()) {
      throw FbossError(
          "if --dynamic_config is passed, --config must have a non-empty argument.");
    }

    auto agentConfig = AgentConfig::fromFile(FLAGS_config);
    auto config = *agentConfig->thrift.sw();
    XLOG(DBG0) << "initialConfig() loaded config from file " << FLAGS_config;

    // If we're passed a config, there's a high probability that it's a prod
    // config and the ports are not in loopback mode.
    for (auto& port : *config.ports()) {
      port.loopbackMode() =
          getAsic()->getDesiredLoopbackMode(port.get_portType());
    }

    return config;
  }

  virtual cfg::SwitchConfig initConfigHelper() const {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
    utility::addProdFeaturesToConfig(
        cfg,
        getHwSwitch()->getPlatform()->getAsic(),
        getHwSwitchEnsemble()->isSai());
    return cfg;
  }

  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    prodInvariants_ = std::make_unique<HwProdInvariantHelper>(
        getHwSwitchEnsemble(), initialConfig());
    prodInvariants_->setupEcmpOnUplinks();
  }

  virtual void verifyInvariants(HwInvariantBitmask options) {
    prodInvariants_->verifyInvariants(options);
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return HwProdInvariantHelper::featuresDesired();
  }

  /*
   * When overridden by subclasses, will return the set of HwInvariantOpts that
   * should be enabled for that role.
   *
   * For example, for HwProdInvariantsRswTest, would return
   * COPP_INVARIANT|OLYMPIC_QOS_INVARIANT|LOAD_BALANCER_INVARIANT.
   */
  virtual HwInvariantBitmask getInvariantOptions() const {
    return DEFAULT_INVARIANTS;
  }

 private:
  std::unique_ptr<HwProdInvariantHelper> prodInvariants_;
};

class HwProdInvariantsRswTest : public HwProdInvariantsTest {
 protected:
  cfg::SwitchConfig initConfigHelper() const override {
    auto config = utility::createProdRswConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getHwSwitchEnsemble()->isSai(),
        false /* Strict priority disabled */);
    return config;
  }

  HwInvariantBitmask getInvariantOptions() const override {
    auto hwAsic = getHwSwitch()->getPlatform()->getAsic();
    HwInvariantBitmask bitmask = DIAG_CMDS_INVARIANT | COPP_INVARIANT;
    if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
      bitmask |= OLYMPIC_QOS_INVARIANT;
    }
    if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
      bitmask |= LOAD_BALANCER_INVARIANT;
    }
    // MPLS on SAI is not supported yet
    if (hwAsic->isSupported(HwAsic::Feature::MPLS) &&
        !getHwSwitchEnsemble()->isSai()) {
      bitmask |= MPLS_INVARIANT;
    }
    return bitmask;
  }
};

TEST_F(HwProdInvariantsRswTest, verifyInvariants) {
  auto verify = [this]() { this->verifyInvariants(getInvariantOptions()); };
  verifyAcrossWarmBoots([]() {}, verify);
}

class HwProdInvariantsRswStrictPriorityTest : public HwProdInvariantsRswTest {
 protected:
  cfg::SwitchConfig initConfigHelper() const override {
    auto config = utility::createProdRswConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getHwSwitchEnsemble()->isSai(),
        true /* Strict priority enabled */);
    return config;
  }
};

TEST_F(HwProdInvariantsRswStrictPriorityTest, verifyInvariants) {
  auto verify = [this]() { this->verifyInvariants(getInvariantOptions()); };
  verifyAcrossWarmBoots([]() {}, verify);
}

class HwProdInvariantsFswTest : public HwProdInvariantsTest {
 protected:
  cfg::SwitchConfig initConfigHelper() const override {
    auto config = utility::createProdFswConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getHwSwitchEnsemble()->isSai(),
        false /* Strict priority disabled */);
    return config;
  }

  HwInvariantBitmask getInvariantOptions() const override {
    auto hwAsic = getHwSwitch()->getPlatform()->getAsic();
    HwInvariantBitmask bitmask = DIAG_CMDS_INVARIANT | COPP_INVARIANT;
    if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
      bitmask |= OLYMPIC_QOS_INVARIANT;
    }
    if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
      bitmask |= LOAD_BALANCER_INVARIANT;
    }
    // MPLS on SAI is not supported yet
    if (hwAsic->isSupported(HwAsic::Feature::MPLS) &&
        !getHwSwitchEnsemble()->isSai()) {
      bitmask |= MPLS_INVARIANT;
    }
    return bitmask;
  }
};

TEST_F(HwProdInvariantsFswTest, verifyInvariants) {
  auto verify = [this]() { this->verifyInvariants(getInvariantOptions()); };
  verifyAcrossWarmBoots([]() {}, verify);
}

class HwProdInvariantsFswStrictPriorityTest : public HwProdInvariantsFswTest {
 protected:
  cfg::SwitchConfig initConfigHelper() const override {
    auto config = utility::createProdFswConfig(
        getHwSwitch(),
        masterLogicalInterfacePortIds(),
        getHwSwitchEnsemble()->isSai(),
        true /* Strict priority enabled */);
    return config;
  }
};

TEST_F(HwProdInvariantsFswStrictPriorityTest, verifyInvariants) {
  auto verify = [this]() { this->verifyInvariants(getInvariantOptions()); };
  verifyAcrossWarmBoots([]() {}, verify);
}

class HwProdInvariantsRswMhnicTest : public HwProdInvariantsTest {
 protected:
  cfg::SwitchConfig initConfigHelper() const override {
    auto config = utility::createProdRswMhnicConfig(
        getHwSwitch(), masterLogicalPortIds(), getHwSwitchEnsemble()->isSai());
    return config;
  }

  HwInvariantBitmask getInvariantOptions() const override {
    auto hwAsic = getHwSwitch()->getPlatform()->getAsic();
    auto bitmask = DIAG_CMDS_INVARIANT | COPP_INVARIANT;
    if (hwAsic->isSupported(HwAsic::Feature::L3_QOS)) {
      bitmask |= MHNIC_INVARIANT;
    }
    if (hwAsic->isSupported(HwAsic::Feature::HASH_FIELDS_CUSTOMIZATION)) {
      bitmask |= LOAD_BALANCER_INVARIANT;
    }
    // MPLS on SAI is not supported yet
    if (hwAsic->isSupported(HwAsic::Feature::MPLS) &&
        !getHwSwitchEnsemble()->isSai()) {
      bitmask |= MPLS_INVARIANT;
    }
    return bitmask;
  }

  void addRoutes(
      const std::vector<RoutePrefix<folly::IPAddressV4>>& routePrefixes) {
    auto kEcmpWidth = 1;
    utility::EcmpSetupAnyNPorts<folly::IPAddressV4> ecmpHelper(
        getProgrammedState(), RouterID(0));

    applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), kEcmpWidth));

    ecmpHelper.programRoutes(getRouteUpdater(), kEcmpWidth, routePrefixes);
  }

  RoutePrefix<folly::IPAddressV4> kGetRoutePrefix() {
    // Currently hardcoded to IPv4. Enabling IPv6 testing for all classes is on
    // the to-do list, after which we can choose v4 or v6 with the same model as
    // kGetRoutePrefix in HwQueuePerHostRouteTests.
    return RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"10.10.1.0"}, 24};
  }
};

TEST_F(HwProdInvariantsRswMhnicTest, verifyInvariants) {
  auto setup = [this]() {
    addRoutes({kGetRoutePrefix()});
    utility::updateRoutesClassID(
        {{kGetRoutePrefix(),
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2}},
        getRouteUpdater().get());
  };
  auto verify = [this]() { this->verifyInvariants(getInvariantOptions()); };
  verifyAcrossWarmBoots(setup, verify);
}

class HwProdInvariantsMmuLosslessTest : public HwProdInvariantsTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::createProdRtswConfig(
        getHwSwitch(), masterLogicalPortIds(), getHwSwitchEnsemble());
    return cfg;
  }

  void SetUp() override {
    FLAGS_mmu_lossless_mode = true;
    FLAGS_qgroup_guarantee_enable = true;
    FLAGS_skip_buffer_reservation = true;

    HwLinkStateDependentTest::SetUp();
    prodInvariants_ = std::make_unique<HwProdRtswInvariantHelper>(
        getHwSwitchEnsemble(), initialConfig());
    // explicitly creating with interfaceMac as nexthop, causes
    // routed pkts to be not dropped when looped to ingress
    prodInvariants_->setupEcmpWithNextHopMac(dstMac());
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return HwProdInvariantHelper::featuresDesired();
  }

  void verifyInvariants(HwInvariantBitmask options) override {
    prodInvariants_->verifyInvariants(options);
    prodInvariants_->verifyNoDiscards();
  }

  void verifyNoDiscards() {
    prodInvariants_->verifyNoDiscards();
  }

  MacAddress dstMac() const {
    return utility::getFirstInterfaceMac(getProgrammedState());
  }

  void sendTrafficInLoop() {
    prodInvariants_->disableTtl();
    // send traffic from port which is not in ecmp
    prodInvariants_->sendTraffic();
    // since ports in ecmp width start from 0, just ensure atleast first port
    // has attained the line rate
    getHwSwitchEnsemble()->waitForLineRateOnPort(
        prodInvariants_->getEcmpPortIds()[0]);
  }

  HwInvariantBitmask getInvariantOptions() const override {
    return DEFAULT_INVARIANTS;
  }

 private:
  std::unique_ptr<HwProdRtswInvariantHelper> prodInvariants_;
};

// validate that running there are no discards during line rate run
// of traffic while doing warm boot
TEST_F(HwProdInvariantsMmuLosslessTest, ValidateMmuLosslessMode) {
  verifyAcrossWarmBoots(
      []() {}, [this]() { verifyInvariants(getInvariantOptions()); });
}

TEST_F(HwProdInvariantsMmuLosslessTest, ValidateWarmBootNoDiscards) {
  auto setup = [=, this]() {
    // run the loop, so traffic continues running
    sendTrafficInLoop();
  };

  auto verify = [=, this]() { verifyNoDiscards(); };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
