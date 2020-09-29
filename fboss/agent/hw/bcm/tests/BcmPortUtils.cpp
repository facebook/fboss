/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwPortUtils.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/platforms/common/PlatformMode.h"

extern "C" {
#include <bcm/port.h>
}

namespace {
bcm_port_t getBcmPort(facebook::fboss::PortID port) {
  return static_cast<bcm_port_t>(port);
}

int getUnit(const facebook::fboss::HwSwitch* hw) {
  CHECK(hw);
  return static_cast<const facebook::fboss::BcmSwitch*>(hw)->getUnit();
}

std::vector<facebook::fboss::cfg::Port>::iterator findCfgPort(
    facebook::fboss::cfg::SwitchConfig* cfg,
    facebook::fboss::PortID portID) {
  return std::find_if(
      cfg->ports_ref()->begin(),
      cfg->ports_ref()->end(),
      [&portID](auto& port) {
        return facebook::fboss::PortID(*port.logicalID_ref()) == portID;
      });
}

void updatePortProfileIDAndName(
    facebook::fboss::cfg::SwitchConfig* config,
    facebook::fboss::PortID portID,
    const facebook::fboss::Platform* platform) {
  auto platformPort = platform->getPlatformPort(portID);
  if (auto entry = platformPort->getPlatformPortEntry()) {
    auto portCfg = findCfgPort(config, portID);
    portCfg->name_ref() = *entry->mapping_ref()->name_ref();
    portCfg->profileID_ref() =
        platformPort->getProfileIDBySpeed(*portCfg->speed_ref());
  } else {
    throw facebook::fboss::FbossError(
        "Port:", portID, " doesn't have PlatformPortEntry");
  }
}
} // namespace

namespace facebook::fboss::utility {
bool portEnabled(const HwSwitch* hw, PortID port) {
  int enable = -1;
  auto rv = bcm_port_enable_get(getUnit(hw), getBcmPort(port), &enable);
  bcmCheckError(rv, "failed to get port enable status");
  CHECK(enable == 0 || enable == 1);
  return (enable == 1);
}

cfg::PortSpeed curPortSpeed(const HwSwitch* hw, PortID port) {
  int curSpeed;
  auto ret = bcm_port_speed_get(getUnit(hw), getBcmPort(port), &curSpeed);
  bcmCheckError(ret, "Failed to get current speed for port");
  return cfg::PortSpeed(curSpeed);
}

void assertPort(
    const HwSwitch* hw,
    PortID port,
    bool enabled,
    cfg::PortSpeed speed) {
  CHECK_EQ(enabled, portEnabled(hw, port));
  if (enabled) {
    // Only verify speed on enabled ports
    CHECK_EQ(
        static_cast<int>(speed),
        static_cast<int>(utility::curPortSpeed(hw, port)));
  }
}

void assertPortStatus(const HwSwitch* hw, PortID port) {
  CHECK(portEnabled(hw, port));
}

void assertPortsLoopbackMode(
    const HwSwitch* hw,
    const std::map<PortID, int>& port2LoopbackMode) {
  for (auto portAndLoopBackMode : port2LoopbackMode) {
    assertPortLoopbackMode(
        hw, portAndLoopBackMode.first, portAndLoopBackMode.second);
  }
}

void assertPortSampleDestination(
    const HwSwitch* hw,
    PortID port,
    int expectedSampleDestination) {
  int sampleDestination;
  auto rv = bcm_port_control_get(
      getUnit(hw),
      getBcmPort(port),
      bcmPortControlSampleIngressDest,
      &sampleDestination);
  bcmCheckError(rv, "Failed to get sample destination for port:", port);
  CHECK_EQ(expectedSampleDestination, sampleDestination);
}

void assertPortsSampleDestination(
    const HwSwitch* hw,
    const std::map<PortID, int>& port2SampleDestination) {
  for (auto portAndSampleDestination : port2SampleDestination) {
    assertPortSampleDestination(
        hw, portAndSampleDestination.first, portAndSampleDestination.second);
  }
}

void assertPortLoopbackMode(
    const HwSwitch* hw,
    PortID port,
    int expectedLoopbackMode) {
  int loopbackMode;
  auto rv = bcm_port_loopback_get(getUnit(hw), getBcmPort(port), &loopbackMode);
  bcmCheckError(rv, "Failed to get loopback mode for port:", port);
  CHECK_EQ(expectedLoopbackMode, loopbackMode);
}

void cleanPortConfig(
    cfg::SwitchConfig* /* config */,
    std::vector<PortID> /* allPortsinGroup */) {
  // no need to remove portCfg not in allPortsinGroup for bcm tests
  // do nothing
}

void enableOneLane(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  bool firstLane = true;
  for (auto portID : allPortsinGroup) {
    auto portCfg = findCfgPort(config, portID);
    portCfg->state_ref() =
        firstLane ? cfg::PortState::ENABLED : cfg::PortState::DISABLED;
    portCfg->speed_ref() = firstLane ? enabledLaneSpeed : disabledLaneSpeed;
    updatePortProfileIDAndName(config, portID, platform);
    if (platform->supportsAddRemovePort() && !firstLane) {
      config->ports_ref()->erase(portCfg);
      auto vlanMemberPort = std::find_if(
          config->vlanPorts_ref()->begin(),
          config->vlanPorts_ref()->end(),
          [portID](auto vlanPort) {
            return static_cast<PortID>(*vlanPort.logicalPort_ref()) == portID;
          });
      if (vlanMemberPort != config->vlanPorts_ref()->end()) {
        config->vlanPorts_ref()->erase(vlanMemberPort);
      }
    }
    firstLane = false;
  }
}

void enableAllLanes(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  for (auto portID : allPortsinGroup) {
    auto portCfg = findCfgPort(config, portID);
    portCfg->speed_ref() = enabledLaneSpeed;
    portCfg->state_ref() = cfg::PortState::ENABLED;
    updatePortProfileIDAndName(config, portID, platform);
  }
}

void enableTwoLanes(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  bool oddLane;
  for (auto portID : allPortsinGroup) {
    auto portCfg = findCfgPort(config, portID);
    oddLane = (portID - allPortsinGroup.front()) % 2 == 0 ? true : false;
    portCfg->state_ref() =
        oddLane ? cfg::PortState::ENABLED : cfg::PortState::DISABLED;
    portCfg->speed_ref() = oddLane ? enabledLaneSpeed : disabledLaneSpeed;
    updatePortProfileIDAndName(config, portID, platform);
    if (platform->supportsAddRemovePort() && !oddLane) {
      config->ports_ref()->erase(portCfg);
      auto vlanMemberPort = std::find_if(
          config->vlanPorts_ref()->begin(),
          config->vlanPorts_ref()->end(),
          [portID](auto vlanPort) {
            return static_cast<PortID>(*vlanPort.logicalPort_ref()) == portID;
          });
      if (vlanMemberPort != config->vlanPorts_ref()->end()) {
        config->vlanPorts_ref()->erase(vlanMemberPort);
      }
    }
  }
}

void assertQUADMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    utility::assertPort(hw, *portItr, true, enabledLaneSpeed);
  }
}

void assertDUALMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  bool odd_lane;
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    odd_lane = (*portItr - allPortsinGroup.front()) % 2 == 0 ? true : false;
    bool enabled = odd_lane ? true : false;
    auto speed = odd_lane ? enabledLaneSpeed : disabledLaneSpeed;
    if (enabled || !hw->getPlatform()->supportsAddRemovePort()) {
      utility::assertPort(hw, *portItr, enabled, speed);
    }
  }
}

void assertSINGLEMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    bool enabled = *portItr == allPortsinGroup.front() ? true : false;
    auto speed = enabled ? enabledLaneSpeed : disabledLaneSpeed;
    if (enabled || !hw->getPlatform()->supportsAddRemovePort()) {
      utility::assertPort(hw, *portItr, enabled, speed);
    }
  }
}

void verifyInterfaceMode(
    PortID portID,
    cfg::PortProfileID profileID,
    Platform* platform) {
  auto* bcmSwitch = static_cast<BcmSwitch*>(platform->getHwSwitch());
  auto platformPort =
      bcmSwitch->getPortTable()->getBcmPort(portID)->getPlatformPort();

  int speed;
  auto rv = bcm_port_speed_get(bcmSwitch->getUnit(), portID, &speed);
  bcmCheckError(rv, "Failed to get current speed for port ", portID);
  if (!platformPort->shouldUsePortResourceAPIs()) {
    bcm_port_if_t curMode = bcm_port_if_t(0);
    auto ret = bcm_port_interface_get(bcmSwitch->getUnit(), portID, &curMode);
    bcmCheckError(
        ret, "Failed to get current interface setting for port ", portID);
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
        bcm_port_resource_get(bcmSwitch->getUnit(), portID, &portResource);
    bcmCheckError(
        ret, "Failed to get current port resource settings for: ", portID);

    const auto profileConf = platform->getPortProfileConfig(profileID);
    if (!profileConf) {
      throw FbossError(
          "Platform doesn't support speed profile: ",
          apache::thrift::util::enumNameSafe(profileID));
    }
    auto expectedPhyLaneConfig = utility::getDesiredPhyLaneConfig(*profileConf);
    EXPECT_EQ(expectedPhyLaneConfig, portResource.phy_lane_config);
  }
}

void verifyTxSettting(
    PortID portID,
    cfg::PortProfileID profileID,
    Platform* platform) {
  if (platform->getMode() == PlatformMode::FAKE_WEDGE ||
      platform->getMode() == PlatformMode::FAKE_WEDGE40) {
    // TODO: skip fake now, add support for TxSettings in fake SDK
    return;
  }
  auto* bcmSwitch = static_cast<BcmSwitch*>(platform->getHwSwitch());
  auto platformPort = platform->getPlatformPort(portID);
  const auto& iphyConfigs = platformPort->getIphyPinConfigs(profileID);
  int minLane = -1;
  for (const auto& iphy : iphyConfigs) {
    if (minLane < 0 || iphy.get_id().get_lane() < minLane) {
      minLane = iphy.get_id().get_lane();
    }
  }
  for (const auto& iphy : iphyConfigs) {
    auto expectedTx = iphy.tx_ref();
    if (!expectedTx.has_value()) {
      continue;
    }

    auto lane = iphy.get_id().get_lane() - minLane;
    auto* bcmPort = bcmSwitch->getPortTable()->getBcmPort(portID);
    auto programmedTx = bcmPort->getTxSettingsForLane(lane);
    EXPECT_EQ(*programmedTx.pre_ref(), *expectedTx->pre_ref());
    EXPECT_EQ(*programmedTx.main_ref(), *expectedTx->main_ref());
    EXPECT_EQ(*programmedTx.post_ref(), *expectedTx->post_ref());
    EXPECT_EQ(*programmedTx.pre2_ref(), *expectedTx->pre2_ref());
    if (auto programmedDC = expectedTx->driveCurrent_ref()) {
      EXPECT_EQ(*programmedDC, *expectedTx->driveCurrent_ref());
    }
  }
}

void verifyLedStatus(HwSwitchEnsemble* ensemble, PortID port, bool up) {
  BcmTestPlatform* platform =
      static_cast<BcmTestPlatform*>(ensemble->getPlatform());
  EXPECT_TRUE(platform->verifyLEDStatus(port, up));
}

void verifyRxSettting(
    PortID /*portID*/,
    cfg::PortProfileID /*profileID*/,
    Platform* /*platform*/) {
  // no rx settings
  return;
}

void verifyFec(
    PortID /*portID*/,
    cfg::PortProfileID /*profileID*/,
    Platform* /*platform*/) {
  // TODO: verify FEC
  return;
}
} // namespace facebook::fboss::utility
