/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

extern "C" {
#include <bcm/port.h>
}

namespace {
bool isFlexModeSupported(
    facebook::fboss::BcmTestPlatform* platform,
    facebook::fboss::FlexPortMode desiredMode) {
  for (auto flexMode : platform->getSupportedFlexPortModes()) {
    if (flexMode == desiredMode) {
      return true;
    }
  }
  return false;
}

void getSflowRates(
    int unit,
    bcm_port_t port,
    int* ingressRate,
    int* egressRate) {
  auto rv = bcm_port_sample_rate_get(unit, port, ingressRate, egressRate);
  facebook::fboss::bcmCheckError(rv, "failed to get port sflow rates");
}

} // namespace

namespace facebook::fboss {

class BcmPortTest : public BcmTest {
 protected:
  std::vector<PortID> initialConfiguredPorts() const {
    return {masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
  }
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }
};

TEST_F(BcmPortTest, PortApplyConfig) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };
  auto verify = [this]() {
    for (auto portId : initialConfiguredPorts()) {
      // check port should exist in portTable_
      ASSERT_TRUE(
          getHwSwitch()->getPortTable()->getBcmPortIf(PortID(portId)) !=
          nullptr);
      utility::assertPortStatus(getHwSwitch(), portId);
      int loopbackMode;
      auto rv = bcm_port_loopback_get(getUnit(), portId, &loopbackMode);
      bcmCheckError(rv, "Failed to get loopback mode for port:", portId);
      EXPECT_EQ(BCM_PORT_LOOPBACK_NONE, loopbackMode);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, PortSflowConfig) {
  auto constexpr kIngressRate = 10;
  auto constexpr kEgressRate = 20;
  auto setup = [this, kIngressRate, kEgressRate]() {
    auto newCfg = initialConfig();
    for (auto portId : initialConfiguredPorts()) {
      auto portCfg = utility::findCfgPort(newCfg, portId);
      portCfg->sFlowIngressRate_ref() = kIngressRate;
      portCfg->sFlowEgressRate_ref() = kEgressRate;
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this, kIngressRate, kEgressRate]() {
    for (auto portId : initialConfiguredPorts()) {
      int ingressSamplingRate, egressSamplingRate;
      getSflowRates(
          getUnit(), portId, &ingressSamplingRate, &egressSamplingRate);
      ASSERT_EQ(kIngressRate, ingressSamplingRate);
      ASSERT_EQ(kEgressRate, egressSamplingRate);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, PortLoopbackMode) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    std::vector<cfg::PortLoopbackMode> loopbackModes = {
        cfg::PortLoopbackMode::MAC, cfg::PortLoopbackMode::PHY};
    for (auto index : {0, 1}) {
      auto portId = initialConfiguredPorts()[index];
      auto portCfg = utility::findCfgPort(newCfg, portId);
      portCfg->loopbackMode_ref() = loopbackModes[index];
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    std::map<PortID, int> port2LoopbackMode = {
        {PortID(masterLogicalPortIds()[0]), BCM_PORT_LOOPBACK_MAC},
        {PortID(masterLogicalPortIds()[1]), BCM_PORT_LOOPBACK_PHY},
    };
    utility::assertPortsLoopbackMode(getHwSwitch(), port2LoopbackMode);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, PortLoopbackModeMAC40G) {
  if (!isFlexModeSupported(getPlatform(), FlexPortMode::ONEX40G)) {
    return;
  }

  if (true) {
    // Changing loopback mode to MAC on a 40G port on trident2 changes
    // the speed to 10G unexpectedly. Ignore this test for now...
    //
    // Broadcom case: CS8832244
    //
    return;
  }

  auto setup = [this]() {
    auto newCfg = initialConfig();
    for (auto portId : initialConfiguredPorts()) {
      auto portCfg = utility::findCfgPort(newCfg, portId);
      portCfg->loopbackMode_ref() = cfg::PortLoopbackMode::MAC;
      utility::updatePortSpeed(
          *getHwSwitch(), newCfg, portId, cfg::PortSpeed::FORTYG);
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    for (auto portId : initialConfiguredPorts()) {
      utility::assertPortLoopbackMode(
          getHwSwitch(), portId, BCM_PORT_LOOPBACK_MAC);
      utility::assertPort(getHwSwitch(), portId, true, cfg::PortSpeed::FORTYG);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, PortLoopbackModePHY40G) {
  if (!isFlexModeSupported(getPlatform(), FlexPortMode::ONEX40G)) {
    return;
  }

  auto setup = [this]() {
    auto newCfg = initialConfig();
    for (auto portId : initialConfiguredPorts()) {
      auto portCfg = utility::findCfgPort(newCfg, portId);
      portCfg->loopbackMode_ref() = cfg::PortLoopbackMode::PHY;
      utility::updatePortSpeed(
          *getHwSwitch(), newCfg, portId, cfg::PortSpeed::FORTYG);
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    for (auto portId : initialConfiguredPorts()) {
      utility::assertPortLoopbackMode(
          getHwSwitch(), portId, BCM_PORT_LOOPBACK_PHY);
      utility::assertPort(getHwSwitch(), portId, true, cfg::PortSpeed::FORTYG);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, PortLoopbackModeMAC100G) {
  if (!isFlexModeSupported(getPlatform(), FlexPortMode::ONEX100G)) {
    return;
  }

  auto setup = [this]() {
    auto newCfg = initialConfig();
    for (auto portId : initialConfiguredPorts()) {
      auto portCfg = utility::findCfgPort(newCfg, portId);
      portCfg->loopbackMode_ref() = cfg::PortLoopbackMode::MAC;
      utility::updatePortSpeed(
          *getHwSwitch(), newCfg, portId, cfg::PortSpeed::HUNDREDG);
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    for (auto portId : initialConfiguredPorts()) {
      utility::assertPortLoopbackMode(
          getHwSwitch(), portId, BCM_PORT_LOOPBACK_MAC);
      utility::assertPort(
          getHwSwitch(), portId, true, cfg::PortSpeed::HUNDREDG);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, PortLoopbackModePHY100G) {
  if (!isFlexModeSupported(getPlatform(), FlexPortMode::ONEX100G)) {
    return;
  }

  auto setup = [this]() {
    auto newCfg = initialConfig();
    for (auto portId : initialConfiguredPorts()) {
      auto portCfg = utility::findCfgPort(newCfg, portId);
      portCfg->loopbackMode_ref() = cfg::PortLoopbackMode::PHY;
      utility::updatePortSpeed(
          *getHwSwitch(), newCfg, portId, cfg::PortSpeed::HUNDREDG);
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    for (auto portId : initialConfiguredPorts()) {
      utility::assertPortLoopbackMode(
          getHwSwitch(), portId, BCM_PORT_LOOPBACK_PHY);
      utility::assertPort(
          getHwSwitch(), portId, true, cfg::PortSpeed::HUNDREDG);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, SampleDestination) {
  // sample destination can't be configured if sflow sampling isn't supported
  if (!getPlatform()->sflowSamplingSupported()) {
    return;
  }
  auto setup = [=]() {
    auto newCfg = initialConfig();
    std::vector<cfg::SampleDestination> sampleDestinations = {
        cfg::SampleDestination::CPU, cfg::SampleDestination::MIRROR};
    for (auto index : {0, 1}) {
      auto portId = initialConfiguredPorts()[index];
      auto portCfg = utility::findCfgPort(newCfg, portId);
      portCfg->sampleDest_ref() = sampleDestinations[index];
      if (sampleDestinations[index] == cfg::SampleDestination::MIRROR) {
        portCfg->ingressMirror_ref() = "sflow";
      }
    }
    cfg::MirrorTunnel tunnel;
    cfg::SflowTunnel sflowTunnel;
    *sflowTunnel.ip_ref() = "10.0.0.1";
    sflowTunnel.udpSrcPort_ref() = 6545;
    sflowTunnel.udpDstPort_ref() = 5343;
    tunnel.sflowTunnel_ref() = sflowTunnel;
    newCfg.mirrors_ref()->resize(1);
    *newCfg.mirrors_ref()[0].name_ref() = "sflow";
    newCfg.mirrors_ref()[0].destination_ref()->tunnel_ref() = tunnel;
    return applyNewConfig(newCfg);
  };
  auto verify = [=]() {
    std::map<PortID, int> port2SampleDestination = {
        {PortID(masterLogicalPortIds()[0]), BCM_PORT_CONTROL_SAMPLE_DEST_CPU},
        {PortID(masterLogicalPortIds()[1]),
         BCM_PORT_CONTROL_SAMPLE_DEST_MIRROR},
    };
    utility::assertPortsSampleDestination(
        getHwSwitch(), port2SampleDestination);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, NoSampleDestinationSet) {
  // sample destination can't be configured if sflow sampling isn't supported
  if (!getPlatform()->sflowSamplingSupported()) {
    return;
  }
  auto setup = [=]() { return applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    std::map<PortID, int> port2SampleDestination = {
        {PortID(masterLogicalPortIds()[0]), BCM_PORT_CONTROL_SAMPLE_DEST_CPU}};
    utility::assertPortsSampleDestination(
        getHwSwitch(), port2SampleDestination);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, IngressMirror) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    newCfg.mirrors_ref()->resize(1);
    *newCfg.mirrors_ref()[0].name_ref() = "mirror";
    cfg::MirrorTunnel tunnel;
    cfg::GreTunnel greTunnel;
    *greTunnel.ip_ref() = "1.1.1.10";
    tunnel.greTunnel_ref() = greTunnel;
    newCfg.mirrors_ref()[0].destination_ref()->tunnel_ref() = tunnel;

    auto portId = masterLogicalPortIds()[1];
    auto portCfg = utility::findCfgPort(newCfg, portId);
    portCfg->ingressMirror_ref() = "mirror";
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    auto* bcmPort = getHwSwitch()->getPortTable()->getBcmPort(
        PortID(masterLogicalPortIds()[1]));
    ASSERT_TRUE(bcmPort->getIngressPortMirror().has_value());
    EXPECT_EQ(bcmPort->getIngressPortMirror().value(), "mirror");
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, EgressMirror) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    newCfg.mirrors_ref()->resize(1);
    *newCfg.mirrors_ref()[0].name_ref() = "mirror";
    cfg::MirrorTunnel tunnel;
    cfg::GreTunnel greTunnel;
    *greTunnel.ip_ref() = "1.1.1.10";
    tunnel.greTunnel_ref() = greTunnel;
    newCfg.mirrors_ref()[0].destination_ref()->tunnel_ref() = tunnel;

    auto portId = masterLogicalPortIds()[1];
    auto portCfg = utility::findCfgPort(newCfg, portId);
    portCfg->egressMirror_ref() = "mirror";
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    auto* bcmPort = getHwSwitch()->getPortTable()->getBcmPort(
        PortID(masterLogicalPortIds()[1]));
    ASSERT_TRUE(bcmPort->getEgressPortMirror().has_value());
    EXPECT_EQ(bcmPort->getEgressPortMirror().value(), "mirror");
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, SampleDestinationMirror) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    cfg::Mirror mirror;
    cfg::MirrorTunnel tunnel;
    cfg::SflowTunnel sflowTunnel;
    *sflowTunnel.ip_ref() = "10.0.0.1";
    sflowTunnel.udpSrcPort_ref() = 6545;
    sflowTunnel.udpDstPort_ref() = 5343;
    tunnel.sflowTunnel_ref() = sflowTunnel;
    *mirror.name_ref() = "mirror";
    mirror.destination_ref()->tunnel_ref() = tunnel;
    newCfg.mirrors_ref()->push_back(mirror);

    auto portId = masterLogicalPortIds()[1];
    auto portCfg = utility::findCfgPort(newCfg, portId);
    portCfg->ingressMirror_ref() = "mirror";
    portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
    return applyNewConfig(newCfg);
  };
  auto verify = [=]() {
    auto* bcmPort = getHwSwitch()->getPortTable()->getBcmPort(
        PortID(masterLogicalPortIds()[1]));
    ASSERT_TRUE(bcmPort->getIngressPortMirror().has_value());
    EXPECT_EQ(bcmPort->getIngressPortMirror().value(), "mirror");
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, SampleDestinationCpu) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    cfg::Mirror mirror;
    cfg::MirrorTunnel tunnel;
    cfg::GreTunnel greTunnel;
    *greTunnel.ip_ref() = "1.1.1.10";
    tunnel.greTunnel_ref() = greTunnel;
    *mirror.name_ref() = "mirror";
    mirror.destination_ref()->tunnel_ref() = tunnel;
    newCfg.mirrors_ref()->push_back(mirror);

    auto portId = masterLogicalPortIds()[1];
    auto portCfg = utility::findCfgPort(newCfg, portId);
    portCfg->ingressMirror_ref() = "mirror";
    portCfg->sampleDest_ref() = cfg::SampleDestination::CPU;
    return applyNewConfig(newCfg);
  };
  auto verify = [=]() {
    auto* bcmPort = getHwSwitch()->getPortTable()->getBcmPort(
        PortID(masterLogicalPortIds()[1]));
    ASSERT_TRUE(bcmPort->getIngressPortMirror().has_value());
    EXPECT_EQ(bcmPort->getIngressPortMirror().value(), "mirror");
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, AssertL3Enabled) {
  // Enable all master ports
  auto setup = [this]() {
    applyNewConfig(utility::oneL3IntfNPortConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC));
  };
  auto verify = [this]() {
    std::array<std::tuple<std::string, bcm_port_control_t>, 2> l3Options = {
        std::make_tuple("bcmPortControlIP4", bcmPortControlIP4),
        std::make_tuple("bcmPortControlIP6", bcmPortControlIP6)};
    for (const auto& port : *getProgrammedState()->getPorts()) {
      if (!port->isEnabled()) {
        continue;
      }
      for (auto l3Option : l3Options) {
        int currVal{0};
        auto rv = bcm_port_control_get(
            getUnit(),
            static_cast<int>(port->getID()),
            std::get<1>(l3Option),
            &currVal);
        bcmCheckError(
            rv,
            folly::sformat(
                "Failed to get {} for port {} : {}",
                std::get<0>(l3Option),
                static_cast<int>(port->getID()),
                bcm_errmsg(rv)));
        // Any enabled port should enable IP4 and IP6
        EXPECT_EQ(currVal, 1);
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
