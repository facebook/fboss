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
#include "fboss/agent/hw/bcm/tests/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
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
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }
};

TEST_F(BcmPortTest, PortApplyConfig) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };
  auto verify = [this]() {
    for (auto index : {0, 1}) {
      auto portId = masterLogicalPortIds()[index];
      // check port should exist in portTable_
      ASSERT_TRUE(
          getHwSwitch()->getPortTable()->getBcmPortIf(PortID(portId)) !=
          nullptr);
      utility::assertPortStatus(getUnit(), portId);
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
    for (auto index : {0, 1}) {
      newCfg.ports[index].sFlowIngressRate = kIngressRate;
      newCfg.ports[index].sFlowEgressRate = kEgressRate;
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this, kIngressRate, kEgressRate]() {
    for (auto index : {0, 1}) {
      int ingressSamplingRate, egressSamplingRate;
      getSflowRates(
          getUnit(),
          masterLogicalPortIds()[index],
          &ingressSamplingRate,
          &egressSamplingRate);
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
      newCfg.ports[index].loopbackMode = loopbackModes[index];
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    std::map<PortID, int> port2LoopbackMode = {
        {PortID(masterLogicalPortIds()[0]), BCM_PORT_LOOPBACK_MAC},
        {PortID(masterLogicalPortIds()[1]), BCM_PORT_LOOPBACK_PHY},
    };
    utility::assertPortsLoopbackMode(getUnit(), port2LoopbackMode);
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
    for (auto index : {0, 1}) {
      newCfg.ports[index].loopbackMode = cfg::PortLoopbackMode::MAC;
      newCfg.ports[index].speed = cfg::PortSpeed::FORTYG;
      newCfg.ports[index].profileID =
          getPlatform()
              ->getPlatformPort(PortID(newCfg.ports[index].logicalID))
              ->getProfileIDBySpeed(newCfg.ports[index].speed);
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    for (auto index : {0, 1}) {
      auto id = masterLogicalPortIds()[index];
      utility::assertPortLoopbackMode(
          getUnit(), PortID(id), BCM_PORT_LOOPBACK_MAC);
      utility::assertPort(getUnit(), id, true, cfg::PortSpeed::FORTYG);
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
    for (auto index : {0, 1}) {
      newCfg.ports[index].loopbackMode = cfg::PortLoopbackMode::PHY;
      newCfg.ports[index].speed = cfg::PortSpeed::FORTYG;
      newCfg.ports[index].profileID =
          getPlatform()
              ->getPlatformPort(PortID(newCfg.ports[index].logicalID))
              ->getProfileIDBySpeed(newCfg.ports[index].speed);
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    for (auto index : {0, 1}) {
      auto id = masterLogicalPortIds()[index];
      utility::assertPortLoopbackMode(
          getUnit(), PortID(id), BCM_PORT_LOOPBACK_PHY);
      utility::assertPort(getUnit(), id, true, cfg::PortSpeed::FORTYG);
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
    for (auto index : {0, 1}) {
      newCfg.ports[index].loopbackMode = cfg::PortLoopbackMode::MAC;
      newCfg.ports[index].speed = cfg::PortSpeed::HUNDREDG;
      newCfg.ports[index].profileID =
          getPlatform()
              ->getPlatformPort(PortID(newCfg.ports[index].logicalID))
              ->getProfileIDBySpeed(newCfg.ports[index].speed);
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    for (auto index : {0, 1}) {
      auto id = masterLogicalPortIds()[index];
      utility::assertPortLoopbackMode(
          getUnit(), PortID(id), BCM_PORT_LOOPBACK_MAC);
      utility::assertPort(getUnit(), id, true, cfg::PortSpeed::HUNDREDG);
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
    for (auto index : {0, 1}) {
      newCfg.ports[index].loopbackMode = cfg::PortLoopbackMode::PHY;
      newCfg.ports[index].speed = cfg::PortSpeed::HUNDREDG;
      newCfg.ports[index].profileID =
          getPlatform()
              ->getPlatformPort(PortID(newCfg.ports[index].logicalID))
              ->getProfileIDBySpeed(newCfg.ports[index].speed);
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    for (auto index : {0, 1}) {
      auto id = masterLogicalPortIds()[index];
      utility::assertPortLoopbackMode(
          getUnit(), PortID(id), BCM_PORT_LOOPBACK_PHY);
      utility::assertPort(getUnit(), id, true, cfg::PortSpeed::HUNDREDG);
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
      newCfg.ports[index].sampleDest_ref() = sampleDestinations[index];
      if (sampleDestinations[index] == cfg::SampleDestination::MIRROR) {
        newCfg.ports[index].ingressMirror_ref() = "sflow";
      }
    }
    cfg::MirrorTunnel tunnel;
    cfg::SflowTunnel sflowTunnel;
    sflowTunnel.ip = "10.0.0.1";
    sflowTunnel.udpSrcPort_ref() = 6545;
    sflowTunnel.udpDstPort_ref() = 5343;
    tunnel.sflowTunnel_ref() = sflowTunnel;
    newCfg.mirrors.resize(1);
    newCfg.mirrors[0].name = "sflow";
    newCfg.mirrors[0].destination.tunnel_ref() = tunnel;
    return applyNewConfig(newCfg);
  };
  auto verify = [=]() {
    std::map<PortID, int> port2SampleDestination = {
        {PortID(masterLogicalPortIds()[0]), BCM_PORT_CONTROL_SAMPLE_DEST_CPU},
        {PortID(masterLogicalPortIds()[1]),
         BCM_PORT_CONTROL_SAMPLE_DEST_MIRROR},
    };
    utility::assertPortsSampleDestination(getUnit(), port2SampleDestination);
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
    utility::assertPortsSampleDestination(getUnit(), port2SampleDestination);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, IngressMirror) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    newCfg.mirrors.resize(1);
    newCfg.mirrors[0].name = "mirror";
    cfg::MirrorTunnel tunnel;
    cfg::GreTunnel greTunnel;
    greTunnel.ip = "1.1.1.10";
    tunnel.greTunnel_ref() = greTunnel;
    newCfg.mirrors[0].destination.tunnel_ref() = tunnel;

    newCfg.ports[1].ingressMirror_ref() = "mirror";
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
    newCfg.mirrors.resize(1);
    newCfg.mirrors[0].name = "mirror";
    cfg::MirrorTunnel tunnel;
    cfg::GreTunnel greTunnel;
    greTunnel.ip = "1.1.1.10";
    tunnel.greTunnel_ref() = greTunnel;
    newCfg.mirrors[0].destination.tunnel_ref() = tunnel;

    newCfg.ports[1].egressMirror_ref() = "mirror";
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
    sflowTunnel.ip = "10.0.0.1";
    sflowTunnel.udpSrcPort_ref() = 6545;
    sflowTunnel.udpDstPort_ref() = 5343;
    tunnel.sflowTunnel_ref() = sflowTunnel;
    mirror.name = "mirror";
    mirror.destination.tunnel_ref() = tunnel;
    newCfg.mirrors.push_back(mirror);

    newCfg.ports[1].ingressMirror_ref() = "mirror";
    newCfg.ports[1].sampleDest_ref() = cfg::SampleDestination::MIRROR;
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
    greTunnel.ip = "1.1.1.10";
    tunnel.greTunnel_ref() = greTunnel;
    mirror.name = "mirror";
    mirror.destination.tunnel_ref() = tunnel;
    newCfg.mirrors.push_back(mirror);

    newCfg.ports[1].ingressMirror_ref() = "mirror";
    newCfg.ports[1].sampleDest_ref() = cfg::SampleDestination::CPU;
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

TEST_F(BcmPortTest, FECStatus100GPort) {
  // For 100G ports FEC is enabled by default, unless the platform port
  // explicitly disabled it.
  if (!isFlexModeSupported(getPlatform(), FlexPortMode::ONEX100G)) {
    return;
  }
  auto setup = [this]() {
    auto newCfg = initialConfig();
    auto platformPort = static_cast<BcmTestPort*>(
        getHwSwitch()
            ->getPortTable()
            ->getBcmPort(PortID(masterLogicalPortIds()[1]))
            ->getPlatformPort());
    platformPort->setShouldDisableFec();
    for (auto index : {0, 1}) {
      newCfg.ports[index].loopbackMode = cfg::PortLoopbackMode::MAC;
      newCfg.ports[index].speed = cfg::PortSpeed::HUNDREDG;
      newCfg.ports[index].profileID =
          getPlatform()
              ->getPlatformPort(PortID(newCfg.ports[index].logicalID))
              ->getProfileIDBySpeed(newCfg.ports[index].speed);
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_TRUE(
        getHwSwitch()->getPortFECEnabled(PortID(masterLogicalPortIds()[0])));
    EXPECT_FALSE(
        getHwSwitch()->getPortFECEnabled(PortID(masterLogicalPortIds()[1])));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, FECStatusNon100GPort) {
  // For non 100G ports enabling FEC is controlled via config.  However
  // platform port explicitly disables it.
  if (!isFlexModeSupported(getPlatform(), FlexPortMode::ONEX40G)) {
    return;
  }
  auto setup = [this]() {
    auto newCfg = initialConfig();
    auto platformPort = static_cast<BcmTestPort*>(
        getHwSwitch()
            ->getPortTable()
            ->getBcmPort(PortID(masterLogicalPortIds()[1]))
            ->getPlatformPort());
    platformPort->setShouldDisableFec();
    for (auto index : {0, 1}) {
      newCfg.ports[index].loopbackMode = cfg::PortLoopbackMode::MAC;
      newCfg.ports[index].speed = cfg::PortSpeed::FORTYG;
      newCfg.ports[index].fec = cfg::PortFEC::ON;
    }
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_TRUE(
        getHwSwitch()->getPortFECEnabled(PortID(masterLogicalPortIds()[0])));
    EXPECT_FALSE(
        getHwSwitch()->getPortFECEnabled(PortID(masterLogicalPortIds()[1])));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPortTest, AssertMode) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };
  auto verify = [this]() {
    for (const auto& port : *getProgrammedState()->getPorts()) {
      if (!port->isEnabled()) {
        continue;
      }
      auto platformPort = getHwSwitch()
                              ->getPortTable()
                              ->getBcmPort(port->getID())
                              ->getPlatformPort();

      int speed;
      auto rv = bcm_port_speed_get(getUnit(), port->getID(), &speed);
      bcmCheckError(
          rv, "Failed to get current speed for port ", port->getName());
      if (!platformPort->shouldUsePortResourceAPIs()) {
        bcm_port_if_t curMode = bcm_port_if_t(0);
        auto ret = bcm_port_interface_get(getUnit(), port->getID(), &curMode);
        bcmCheckError(
            ret,
            "Failed to get current interface setting for port ",
            port->getName());
        // TODO - Feed other transmitter technology, speed and build a
        // exhaustive set of speed, transmitter tech to test for
        auto expectedMode = getSpeedToTransmitterTechAndMode()
                                .at(cfg::PortSpeed(speed))
                                .at(platformPort->getTransmitterTech().value());
        EXPECT_EQ(expectedMode, curMode);
      } else {
        // On platforms that use port resource APIs, phy lane config determines
        // the interface mode.
        bcm_port_resource_t portResource;
        auto ret =
            bcm_port_resource_get(getUnit(), port->getID(), &portResource);
        bcmCheckError(
            ret,
            "Failed to get current port resource settings for: ",
            port->getName());

        int expectedPhyLaneConfig = 0;
        if (const auto profileConf =
                getPlatform()->getPortProfileConfig(port->getProfileID())) {
          expectedPhyLaneConfig = utility::getDesiredPhyLaneConfig(
              profileConf->get_iphy().get_modulation(),
              platformPort->getTransmitterTech().value());
        } else {
          // TODO(@ccpowers) We'll support Minipack/Yamp platform using
          // PlatformMapping once we can support generate complete test config.
          expectedPhyLaneConfig = getDesiredPhyLaneConfig(
              platformPort->getTransmitterTech().value(),
              cfg::PortSpeed(speed));
        }
        EXPECT_EQ(expectedPhyLaneConfig, portResource.phy_lane_config);
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
