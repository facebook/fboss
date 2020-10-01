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

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortUtils.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/platforms/common/utils/GalaxyLedUtils.h"
#include "fboss/agent/platforms/common/utils/Wedge100LedUtils.h"
#include "fboss/agent/platforms/common/utils/Wedge400LedUtils.h"
#include "fboss/agent/platforms/common/utils/Wedge40LedUtils.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"

#include "fboss/agent/FbossError.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

namespace {
SaiPortTraits::AdapterKey getPortAdapterKey(const HwSwitch* hw, PortID port) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto handle = saiSwitch->managerTable()->portManager().getPortHandle(port);
  CHECK(handle);
  return handle->port->adapterKey();
}

std::pair<std::string, cfg::PortProfileID> getMappingNameAndProfileID(
    const Platform* platform,
    PortID port,
    cfg::PortSpeed speed) {
  auto platformPort = platform->getPlatformPort(port);
  if (auto entry = platformPort->getPlatformPortEntry()) {
    return {*entry->mapping_ref()->name_ref(),
            platformPort->getProfileIDBySpeed(speed)};

  } else {
    throw FbossError("Port:", port, " doesn't have PlatformPortEntry");
  }
}

std::vector<sai_uint32_t> getTxSetting(
    const std::vector<phy::TxSettings>& tx,
    std::function<sai_uint32_t(phy::TxSettings)> func) {
  std::vector<sai_uint32_t> result{};
  std::transform(tx.begin(), tx.end(), std::back_inserter(result), func);
  return result;
}

std::vector<sai_int32_t> getRxSetting(
    const std::vector<phy::RxSettings>& rx,
    std::function<sai_uint32_t(phy::RxSettings)> func) {
  std::vector<sai_int32_t> result{};
  std::transform(rx.begin(), rx.end(), std::back_inserter(result), func);
  return result;
}
} // namespace
bool portEnabled(const HwSwitch* hw, PortID port) {
  auto key = getPortAdapterKey(hw, port);
  SaiPortTraits::Attributes::AdminState state;
  SaiApiTable::getInstance()->portApi().getAttribute(key, state);
  return state.value();
}

cfg::PortSpeed curPortSpeed(const HwSwitch* hw, PortID port) {
  auto key = getPortAdapterKey(hw, port);
  SaiPortTraits::Attributes::Speed speed;
  SaiApiTable::getInstance()->portApi().getAttribute(key, speed);
  return cfg::PortSpeed(speed.value());
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
    const HwSwitch* /*hw*/,
    PortID /*port*/,
    int /*expectedSampleDestination*/) {
  throw FbossError("sampling is unsupported for SAI");
}

void assertPortsSampleDestination(
    const HwSwitch* /*hw*/,
    const std::map<PortID, int>& /*port2SampleDestination*/) {
  throw FbossError("sampling is unsupported for SAI");
}

void assertPortLoopbackMode(
    const HwSwitch* hw,
    PortID port,
    int expectedLoopbackMode) {
  auto key = getPortAdapterKey(hw, port);
  SaiPortTraits::Attributes::InternalLoopbackMode loopbackMode;
  SaiApiTable::getInstance()->portApi().getAttribute(key, loopbackMode);
  CHECK_EQ(expectedLoopbackMode, loopbackMode.value());
}

void cleanPortConfig(
    cfg::SwitchConfig* config,
    std::vector<PortID> allPortsinGroup) {
  // remove portCfg not in allPortsinGroup
  auto removed = std::remove_if(
      config->ports_ref()->begin(),
      config->ports_ref()->end(),
      [&allPortsinGroup](auto portCfg) {
        auto portID = static_cast<PortID>(*portCfg.logicalID_ref());
        for (auto id : allPortsinGroup) {
          if (portID == id) {
            return false;
          }
        }
        return true;
      });
  config->ports_ref()->erase(removed, config->ports_ref()->end());
}

void enableOneLane(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed /*disabledLaneSpeed*/,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  // remove all except first
  for (auto itr = allPortsinGroup.begin() + 1; itr != allPortsinGroup.end();
       itr++) {
    auto groupMemberPort = std::find_if(
        config->ports_ref()->begin(),
        config->ports_ref()->end(),
        [itr](auto port) {
          return static_cast<PortID>(*port.logicalID_ref()) == *itr;
        });
    config->ports_ref()->erase(groupMemberPort);
    auto vlanMemberPort = std::find_if(
        config->vlanPorts_ref()->begin(),
        config->vlanPorts_ref()->end(),
        [itr](auto vlanPort) {
          return static_cast<PortID>(*vlanPort.logicalPort_ref()) == *itr;
        });
    config->vlanPorts_ref()->erase(vlanMemberPort);
  }

  auto firstLanePort = std::find_if(
      config->ports_ref()->begin(),
      config->ports_ref()->end(),
      [&allPortsinGroup](auto port) {
        return static_cast<PortID>(*port.logicalID_ref()) == allPortsinGroup[0];
      });
  *firstLanePort->speed_ref() = enabledLaneSpeed;
  *firstLanePort->state_ref() = cfg::PortState::ENABLED;

  auto [name, profileID] = getMappingNameAndProfileID(
      platform,
      static_cast<PortID>(*firstLanePort->logicalID_ref()),
      enabledLaneSpeed);
  firstLanePort->name_ref() = name;
  *firstLanePort->profileID_ref() = profileID;
}

void enableAllLanes(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  // keep all except
  for (auto portID : allPortsinGroup) {
    auto port = std::find_if(
        config->ports_ref()->begin(),
        config->ports_ref()->end(),
        [&portID](auto portCfg) {
          return static_cast<PortID>(*portCfg.logicalID_ref()) == portID;
        });
    port->speed_ref() = enabledLaneSpeed;
    port->state_ref() = cfg::PortState::ENABLED;

    auto [name, profileID] =
        getMappingNameAndProfileID(platform, portID, enabledLaneSpeed);
    port->name_ref() = name;
    port->profileID_ref() = profileID;
  }
}

void enableTwoLanes(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed /*disabledLaneSpeed*/,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  // keep only first and third
  auto front = allPortsinGroup.front();
  for (auto portID : allPortsinGroup) {
    auto port = std::find_if(
        config->ports_ref()->begin(),
        config->ports_ref()->end(),
        [&portID](auto portCfg) {
          return static_cast<PortID>(*portCfg.logicalID_ref()) == portID;
        });
    if ((static_cast<PortID>(portID) % 2) != 0) {
      config->ports_ref()->erase(port);
    } else {
      port->speed_ref() = enabledLaneSpeed;
      port->state_ref() = cfg::PortState::ENABLED;

      auto [name, profileID] =
          getMappingNameAndProfileID(platform, portID, enabledLaneSpeed);
      port->name_ref() = name;
      port->profileID_ref() = profileID;
    }
  }

  auto vlanPortItr = config->vlanPorts_ref()->begin();
  while (vlanPortItr != config->vlanPorts_ref()->end()) {
    if ((static_cast<PortID>(*vlanPortItr->logicalPort_ref()) - front) % 2 !=
        0) {
      vlanPortItr = config->vlanPorts_ref()->erase(vlanPortItr);
    } else {
      vlanPortItr++;
    }
  }
}

void assertQUADMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  utility::assertPort(hw, allPortsinGroup[0], true, enabledLaneSpeed);
}

void assertDUALMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed /*disabledLaneSpeed*/,
    std::vector<PortID> allPortsinGroup) {
  bool odd_lane;
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    odd_lane = (*portItr - allPortsinGroup.front()) % 2 == 0 ? true : false;
    if (odd_lane) {
      utility::assertPort(hw, *portItr, true, enabledLaneSpeed);
    }
  }
}
void assertSINGLEMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed /*disabledLaneSpeed*/,
    std::vector<PortID> allPortsinGroup) {
  utility::assertPort(hw, allPortsinGroup[0], true, enabledLaneSpeed);
}

void verifyInterfaceMode(
    PortID portID,
    cfg::PortProfileID /*profileID*/,
    Platform* platform) {
  auto* saiPlatform = static_cast<SaiPlatform*>(platform);
  if (!saiPlatform->supportInterfaceType()) {
    return;
  }
  auto* saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  auto* saiPlatformPort =
      static_cast<SaiPlatformPort*>(saiPlatform->getPlatformPort(portID));
  auto* saiPortHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(portID);

  auto& portApi = SaiApiTable::getInstance()->portApi();
  auto speed = portApi.getAttribute(
      saiPortHandle->port->adapterKey(), SaiPortTraits::Attributes::Speed{});

  auto expectedInterfaceType = saiPlatform->getInterfaceType(
      saiPlatformPort->getTransmitterTech(),
      static_cast<cfg::PortSpeed>(speed));
  auto programmedInterfaceType = portApi.getAttribute(
      saiPortHandle->port->adapterKey(),
      SaiPortTraits::Attributes::InterfaceType{});
  EXPECT_EQ(expectedInterfaceType, programmedInterfaceType);
}

void verifyTxSettting(
    PortID portID,
    cfg::PortProfileID profileID,
    Platform* platform) {
  auto* saiPlatform = static_cast<SaiPlatform*>(platform);
  if (!saiPlatform->isSerdesApiSupported()) {
    return;
  }
  auto* saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  auto* saiPortHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(portID);
  auto serdes = saiPortHandle->serdes;

  auto txSettings = saiPlatform->getPlatformPortTxSettings(portID, profileID);

  if (!serdes) {
    EXPECT_TRUE(txSettings.empty());
    return;
  }
  auto& portApi = SaiApiTable::getInstance()->portApi();
  auto pre = portApi.getAttribute(
      serdes->adapterKey(), SaiPortSerdesTraits::Attributes::TxFirPre1{});
  auto main = portApi.getAttribute(
      serdes->adapterKey(), SaiPortSerdesTraits::Attributes::TxFirMain{});
  auto post = portApi.getAttribute(
      serdes->adapterKey(), SaiPortSerdesTraits::Attributes::TxFirPost1{});

  auto expectedPre = getTxSetting(txSettings, [](phy::TxSettings tx) {
    return static_cast<sai_uint32_t>(*tx.pre_ref());
  });
  auto expectedMain = getTxSetting(txSettings, [](phy::TxSettings tx) {
    return static_cast<sai_uint32_t>(*tx.main_ref());
  });
  auto expectedPost = getTxSetting(txSettings, [](phy::TxSettings tx) {
    return static_cast<sai_uint32_t>(*tx.post_ref());
  });
  auto expectedDriverCurrent =
      getTxSetting(txSettings, [](phy::TxSettings tx) -> sai_uint32_t {
        if (auto driveCurrent = tx.driveCurrent_ref()) {
          return driveCurrent.value();
        }
        return 0;
      });
  EXPECT_EQ(pre, expectedPre);
  EXPECT_EQ(main, expectedMain);
  EXPECT_EQ(post, expectedPost);
  if (txSettings[0].driveCurrent_ref()) {
    auto driverCurrent = portApi.getAttribute(
        serdes->adapterKey(), SaiPortSerdesTraits::Attributes::IDriver{});
    EXPECT_EQ(driverCurrent, expectedDriverCurrent);
  }
}

void verifyLedStatus(HwSwitchEnsemble* ensemble, PortID port, bool up) {
  SaiPlatform* platform = static_cast<SaiPlatform*>(ensemble->getPlatform());
  SaiPlatformPort* platformPort = platform->getPort(port);
  uint32_t currentVal = platformPort->getCurrentLedState();
  uint32_t expectedVal = 0;
  switch (platform->getMode()) {
    case PlatformMode::WEDGE: {
      expectedVal =
          static_cast<uint32_t>(Wedge40LedUtils::getExpectedLEDState(up, up));
    } break;
    case PlatformMode::WEDGE100: {
      expectedVal = static_cast<uint32_t>(Wedge100LedUtils::getExpectedLEDState(
          platform->getLaneCount(platformPort->getCurrentProfile()), up, up));
    } break;
    case PlatformMode::GALAXY_FC:
    case PlatformMode::GALAXY_LC: {
      expectedVal = GalaxyLedUtils::getExpectedLEDState(up, up);
    } break;
    case PlatformMode::WEDGE400C: {
      expectedVal = static_cast<uint32_t>(Wedge400LedUtils::getLedState(
          platform->getLaneCount(platformPort->getCurrentProfile()), up, up));
    } break;
    default:
      return;
  }
  EXPECT_EQ(currentVal, expectedVal);
}

void verifyRxSettting(
    PortID portID,
    cfg::PortProfileID profileID,
    Platform* platform) {
  auto* saiPlatform = static_cast<SaiPlatform*>(platform);
  if (!saiPlatform->isSerdesApiSupported()) {
    return;
  }
  auto* saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  auto* saiPortHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(portID);
  auto serdes = saiPortHandle->serdes;

  auto rxSettings = saiPlatform->getPlatformPortRxSettings(portID, profileID);
  if (!serdes) {
    EXPECT_TRUE(rxSettings.empty());
    return;
  }
  if (rxSettings.empty()) {
    // not all platforms may have these settings
    return;
  }
  auto& portApi = SaiApiTable::getInstance()->portApi();
  if (rxSettings[0].ctlCode_ref()) {
    auto rxCtlCode = portApi.getAttribute(
        serdes->adapterKey(), SaiPortSerdesTraits::Attributes::RxCtleCode{});
    auto expectedRxCtlCode = getRxSetting(rxSettings, [](phy::RxSettings rx) {
      return static_cast<sai_int32_t>(*rx.ctlCode_ref());
    });
    EXPECT_EQ(rxCtlCode, expectedRxCtlCode);
  }
  if (rxSettings[0].dspMode_ref()) {
    auto rxDspMode = portApi.getAttribute(
        serdes->adapterKey(), SaiPortSerdesTraits::Attributes::RxDspMode{});
    auto expectedRxDspMode = getRxSetting(rxSettings, [](phy::RxSettings rx) {
      return static_cast<sai_int32_t>(*rx.dspMode_ref());
    });
    EXPECT_EQ(rxDspMode, expectedRxDspMode);
  }
  if (rxSettings[0].afeTrim_ref()) {
    auto rxAafeTrim = portApi.getAttribute(
        serdes->adapterKey(), SaiPortSerdesTraits::Attributes::RxAfeTrim{});
    auto expectedRxAfeTrim = getRxSetting(rxSettings, [](phy::RxSettings rx) {
      return static_cast<sai_int32_t>(*rx.afeTrim_ref());
    });
    EXPECT_EQ(rxAafeTrim, expectedRxAfeTrim);
  }
  if (rxSettings[0].acCouplingBypass_ref()) {
    auto rxAcCouplingBypass = portApi.getAttribute(
        serdes->adapterKey(),
        SaiPortSerdesTraits::Attributes::RxAcCouplingByPass{});
    auto expectedRxAcCouplingBypass =
        getRxSetting(rxSettings, [](phy::RxSettings rx) {
          return static_cast<sai_int32_t>(*rx.acCouplingBypass_ref());
        });
    EXPECT_EQ(rxAcCouplingBypass, expectedRxAcCouplingBypass);
  }
}

void verifyFec(
    PortID portID,
    cfg::PortProfileID profileID,
    Platform* platform) {
  auto* saiPlatform = static_cast<SaiPlatform*>(platform);
  auto* saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  auto* saiPortHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(portID);

  // retrive configured fec.
  auto expectedFec =
      utility::getSaiPortFecMode(platform->getPhyFecMode(profileID));

  // retrive programmed fec.
  auto& portApi = SaiApiTable::getInstance()->portApi();
  auto programmedFec = portApi.getAttribute(
      saiPortHandle->port->adapterKey(), SaiPortTraits::Attributes::FecMode{});

  EXPECT_EQ(expectedFec, programmedFec);
}
} // namespace facebook::fboss::utility
