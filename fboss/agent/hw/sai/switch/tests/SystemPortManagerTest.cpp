/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiSystemPortManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPort.h"
#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

std::shared_ptr<SystemPort> makeSystemPort(
    const std::optional<std::string>& qosPolicy,
    int64_t sysPortId = 1) {
  auto sysPort = std::make_shared<SystemPort>(SystemPortID(sysPortId));
  sysPort->setSwitchId(SwitchID(1));
  sysPort->setPortName("sysPort1");
  sysPort->setCoreIndex(42);
  sysPort->setCorePortIndex(24);
  sysPort->setSpeedMbps(10000);
  sysPort->setNumVoqs(8);
  sysPort->setEnabled(true);
  sysPort->setQosPolicy(qosPolicy);
  return sysPort;
}

class SystemPortManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT;
    ManagerTestBase::SetUp();
  }
};

TEST_F(SystemPortManagerTest, addSystemPort) {
  std::shared_ptr<SystemPort> swSystemPort = makeSystemPort(std::nullopt);
  SystemPortSaiId saiId =
      saiManagerTable->systemPortManager().addSystemPort(swSystemPort);
  auto configInfo = saiApiTable->systemPortApi().getAttribute(
      saiId, SaiSystemPortTraits::Attributes::ConfigInfo());
  EXPECT_EQ(configInfo.port_id, 1);
}

TEST_F(SystemPortManagerTest, addTwoSystemPort) {
  SystemPortSaiId saiId = saiManagerTable->systemPortManager().addSystemPort(
      makeSystemPort(std::nullopt));
  auto configInfo = saiApiTable->systemPortApi().getAttribute(
      saiId, SaiSystemPortTraits::Attributes::ConfigInfo());
  EXPECT_EQ(configInfo.port_id, 1);
  SystemPortSaiId saiId2 = saiManagerTable->systemPortManager().addSystemPort(
      makeSystemPort(std::nullopt, 2));
  auto configInfo2 = saiApiTable->systemPortApi().getAttribute(
      saiId2, SaiSystemPortTraits::Attributes::ConfigInfo());
  EXPECT_EQ(configInfo2.port_id, 2);
}

TEST_F(SystemPortManagerTest, addDupSystemPort) {
  saiManagerTable->systemPortManager().addSystemPort(
      makeSystemPort(std::nullopt));
  EXPECT_THROW(
      saiManagerTable->systemPortManager().addSystemPort(
          makeSystemPort(std::nullopt)),
      FbossError);
}

TEST_F(SystemPortManagerTest, addSystemPortViaSwitchState) {
  std::shared_ptr<SystemPort> swSystemPort = makeSystemPort(std::nullopt);
  auto state = programmedState->clone();
  state->addSystemPort(swSystemPort);
  applyNewState(state);
  auto handle =
      saiManagerTable->systemPortManager().getSystemPortHandle(SystemPortID(1));
  EXPECT_NE(handle, nullptr);
  auto configInfo =
      GET_ATTR(SystemPort, ConfigInfo, handle->systemPort->attributes());
  EXPECT_EQ(configInfo.port_id, 1);
}
