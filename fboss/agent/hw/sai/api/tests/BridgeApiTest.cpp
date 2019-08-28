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
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<BridgeApi> bridgeApi;
  void checkBridge(BridgeSaiId bridgeId) {
    auto fb = fs->brm.get(bridgeId);
    EXPECT_EQ(fb.id, bridgeId);
  }
  void checkBridgePort(BridgePortSaiId bridgePortId) {
    auto fbp = fs->brm.getMember(bridgePortId);
    EXPECT_EQ(fbp.id, bridgePortId);
  }
};

TEST_F(BridgeApiTest, createBridge) {
  SaiBridgeTraits::Attributes::Type bridgeType(SAI_BRIDGE_TYPE_1Q);
  auto bridgeId = bridgeApi->create2<SaiBridgeTraits>({bridgeType}, 0);
  checkBridge(bridgeId);
}

TEST_F(BridgeApiTest, removeBridge) {
  SaiBridgeTraits::Attributes::Type bridgeType(SAI_BRIDGE_TYPE_1Q);
  auto bridgeId = bridgeApi->create2<SaiBridgeTraits>({bridgeType}, 0);
  checkBridge(bridgeId);
  // account for default bridge
  EXPECT_EQ(fs->brm.map().size(), 2);
  bridgeApi->remove2(bridgeId);
  EXPECT_EQ(fs->brm.map().size(), 1);
}

TEST_F(BridgeApiTest, createBridgePort) {
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT, 42};
  auto bridgePortId = bridgeApi->create2<SaiBridgePortTraits>(c, 0);
  checkBridgePort(bridgePortId);
}

TEST_F(BridgeApiTest, removeBridgePort) {
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT, 42};
  auto bridgePortId = bridgeApi->create2<SaiBridgePortTraits>(c, 0);
  checkBridgePort(bridgePortId);
  EXPECT_EQ(fs->brm.get(0).fm().map().size(), 1);
  bridgeApi->remove2(bridgePortId);
  EXPECT_EQ(fs->brm.get(0).fm().map().size(), 0);
}
