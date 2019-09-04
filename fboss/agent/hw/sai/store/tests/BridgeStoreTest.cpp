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
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <variant>

using namespace facebook::fboss;

class BridgeStoreTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    saiApiTable = SaiApiTable::getInstance();
    saiApiTable->queryApis();
  }
  std::shared_ptr<FakeSai> fs;
  std::shared_ptr<SaiApiTable> saiApiTable;
};

TEST_F(BridgeStoreTest, loadBridge) {
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiBridgeTraits>();
  auto got = store.get(std::monostate{});
  EXPECT_EQ(
      std::get<SaiBridgeTraits::Attributes::Type>(got->attributes()).value(),
      SAI_BRIDGE_TYPE_1Q);
}

TEST_F(BridgeStoreTest, loadBridgePort) {
  auto& bridgeApi = saiApiTable->bridgeApi();
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT, 42};
  auto bridgePortId = bridgeApi.create2<SaiBridgePortTraits>(c, 0);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiBridgePortTraits>();

  auto got = store.get(SaiBridgePortTraits::Attributes::PortId{42});
  EXPECT_EQ(got->adapterKey(), bridgePortId);
}

/*
 * Note, the default bridge id is a pain to get at, so we will skip
 * the bridgeLoadCtor test. That will largely be covered by testing
 * BridgeStore.reload() anyway.
 */

TEST_F(BridgeStoreTest, bridgePortLoadCtor) {
  auto& bridgeApi = saiApiTable->bridgeApi();
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT, 42};
  auto bridgePortId = bridgeApi.create2<SaiBridgePortTraits>(c, 0);
  SaiObject<SaiBridgePortTraits> obj(bridgePortId);
  EXPECT_EQ(obj.adapterKey(), bridgePortId);
  EXPECT_EQ(GET_ATTR(BridgePort, PortId, obj.attributes()), 42);
}

TEST_F(BridgeStoreTest, bridgePortCreateCtor) {
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT, 42};
  SaiObject<SaiBridgePortTraits> obj({42}, c, 0);
  EXPECT_EQ(GET_ATTR(BridgePort, PortId, obj.attributes()), 42);
}
