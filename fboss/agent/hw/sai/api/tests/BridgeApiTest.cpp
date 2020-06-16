/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/BridgeApi.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class BridgeApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    bridgeApi = std::make_unique<BridgeApi>();
  }

  BridgeSaiId createBridge(const sai_bridge_type_t bridgeType) const {
    return bridgeApi->create<SaiBridgeTraits>({bridgeType}, 0);
  }

  BridgePortSaiId createBridgePort(
      const sai_bridge_port_type_t bridgePortType,
      const sai_bridge_port_fdb_learning_mode_t bridgePortFdbLearningMode)
      const {
    SaiBridgePortTraits::Attributes::PortId bridgePortId{42};
    SaiBridgePortTraits::Attributes::AdminState bridgePortAdminState{true};
    SaiBridgePortTraits::CreateAttributes c{bridgePortType,
                                            bridgePortId,
                                            bridgePortAdminState,
                                            bridgePortFdbLearningMode};
    return bridgeApi->create<SaiBridgePortTraits>(c, 0);
  }

  void checkBridge(BridgeSaiId bridgeId) const {
    auto fb = fs->bridgeManager.get(bridgeId);
    EXPECT_EQ(fb.id, bridgeId);
  }
  void checkBridgePort(BridgePortSaiId bridgePortId) const {
    auto fbp = fs->bridgeManager.getMember(bridgePortId);
    EXPECT_EQ(fbp.id, bridgePortId);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<BridgeApi> bridgeApi;
};

TEST_F(BridgeApiTest, createBridge) {
  auto bridgeId = createBridge(SAI_BRIDGE_TYPE_1Q);
  checkBridge(bridgeId);
}

TEST_F(BridgeApiTest, createBridgePort) {
  auto bridgePortId = createBridgePort(
      SAI_BRIDGE_PORT_TYPE_PORT, SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW);
  checkBridgePort(bridgePortId);
}

TEST_F(BridgeApiTest, removeBridgePort) {
  auto bridgePortId = createBridgePort(
      SAI_BRIDGE_PORT_TYPE_PORT, SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW);
  checkBridgePort(bridgePortId);
  EXPECT_EQ(fs->bridgeManager.get(0).fm().map().size(), 1);
  bridgeApi->remove(bridgePortId);
  EXPECT_EQ(fs->bridgeManager.get(0).fm().map().size(), 0);
}

TEST_F(BridgeApiTest, bridgeCount) {
  uint32_t count = getObjectCount<SaiBridgeTraits>(0);
  // default bridge
  EXPECT_EQ(count, 1);
  createBridge(SAI_BRIDGE_TYPE_1Q);
  count = getObjectCount<SaiBridgeTraits>(0);
  EXPECT_EQ(count, 2);
}

TEST_F(BridgeApiTest, bridgePortCount) {
  uint32_t count = getObjectCount<SaiBridgePortTraits>(0);
  EXPECT_EQ(count, 0);
  createBridgePort(
      SAI_BRIDGE_PORT_TYPE_PORT, SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW);
  count = getObjectCount<SaiBridgePortTraits>(0);
  EXPECT_EQ(count, 1);
}

TEST_F(BridgeApiTest, getBridgeAttribute) {
  auto bridgeId = createBridge(SAI_BRIDGE_TYPE_1Q);
  checkBridge(bridgeId);

  auto bridgeTypeGot =
      bridgeApi->getAttribute(bridgeId, SaiBridgeTraits::Attributes::Type());
  EXPECT_EQ(bridgeTypeGot, SAI_BRIDGE_TYPE_1Q);

  EXPECT_THROW(
      bridgeApi->getAttribute(
          bridgeId, SaiBridgeTraits::Attributes::PortList()),
      SaiApiError);
}

TEST_F(BridgeApiTest, setBridgeAttribute) {
  auto bridgeId = createBridge(SAI_BRIDGE_TYPE_1Q);
  checkBridge(bridgeId);

  SaiBridgeTraits::Attributes::Type type{SAI_BRIDGE_TYPE_1Q};
  SaiBridgeTraits::Attributes::PortList portList{};

  // SAI spec does not support setting any attribute for bridge post
  // creation.
  EXPECT_THROW(bridgeApi->setAttribute(bridgeId, type), SaiApiError);
  EXPECT_THROW(bridgeApi->setAttribute(bridgeId, portList), SaiApiError);
}

TEST_F(BridgeApiTest, getBridgePortAttribute) {
  auto bridgePortId = createBridgePort(
      SAI_BRIDGE_PORT_TYPE_PORT, SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW);
  checkBridgePort(bridgePortId);

  auto bridgePortBridgeIdGot = bridgeApi->getAttribute(
      bridgePortId, SaiBridgePortTraits::Attributes::BridgeId());
  auto bridgePortIdGot = bridgeApi->getAttribute(
      bridgePortId, SaiBridgePortTraits::Attributes::PortId());
  auto bridgePortTypeGot = bridgeApi->getAttribute(
      bridgePortId, SaiBridgePortTraits::Attributes::Type());
  auto bridgePortFdbLearningModeGot = bridgeApi->getAttribute(
      bridgePortId, SaiBridgePortTraits::Attributes::FdbLearningMode());
  auto bridgePortAdminStateGot = bridgeApi->getAttribute(
      bridgePortId, SaiBridgePortTraits::Attributes::AdminState());

  EXPECT_EQ(bridgePortBridgeIdGot, 0);
  EXPECT_EQ(bridgePortIdGot, 42);
  EXPECT_EQ(bridgePortTypeGot, SAI_BRIDGE_PORT_TYPE_PORT);
  EXPECT_EQ(bridgePortFdbLearningModeGot, SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW);
  EXPECT_EQ(bridgePortAdminStateGot, true);
}

TEST_F(BridgeApiTest, setBridgePortAttribute) {
  auto bridgePortId = createBridgePort(
      SAI_BRIDGE_PORT_TYPE_PORT, SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW);
  checkBridgePort(bridgePortId);

  SaiBridgePortTraits::Attributes::FdbLearningMode fdbLearningMode{
      SAI_BRIDGE_PORT_FDB_LEARNING_MODE_DISABLE};
  SaiBridgePortTraits::Attributes::AdminState adminState{false};

  bridgeApi->setAttribute(bridgePortId, fdbLearningMode);
  bridgeApi->setAttribute(bridgePortId, adminState);

  auto bridgePortFdbLearningModeGot = bridgeApi->getAttribute(
      bridgePortId, SaiBridgePortTraits::Attributes::FdbLearningMode());
  auto bridgePortAdminStateGot = bridgeApi->getAttribute(
      bridgePortId, SaiBridgePortTraits::Attributes::AdminState());

  EXPECT_EQ(
      bridgePortFdbLearningModeGot, SAI_BRIDGE_PORT_FDB_LEARNING_MODE_DISABLE);
  EXPECT_EQ(bridgePortAdminStateGot, false);

  // SAI spec does not support setting portId and type for bridge port post
  // creation.

  SaiBridgePortTraits::Attributes::PortId portId{101010}; // 42 in binary
  SaiBridgePortTraits::Attributes::Type type{SAI_BRIDGE_PORT_TYPE_SUB_PORT};

  EXPECT_THROW(bridgeApi->setAttribute(bridgePortId, portId), SaiApiError);
  EXPECT_THROW(bridgeApi->setAttribute(bridgePortId, type), SaiApiError);
}
