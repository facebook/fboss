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
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <variant>

using namespace facebook::fboss;

TEST_F(SaiStoreTest, loadBridge) {
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiBridgeTraits>();
  auto got = store.get(std::monostate{});
  EXPECT_EQ(
      std::get<SaiBridgeTraits::Attributes::Type>(got->attributes()).value(),
      SAI_BRIDGE_TYPE_1Q);
}

TEST_F(SaiStoreTest, loadBridgeFromJson) {
  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  SaiStore s2(0);
  s2.reload(&json);
  auto& store = s2.get<SaiBridgeTraits>();
  auto got = store.get(std::monostate{});
  EXPECT_EQ(
      std::get<SaiBridgeTraits::Attributes::Type>(got->attributes()).value(),
      SAI_BRIDGE_TYPE_1Q);
}

TEST_F(SaiStoreTest, loadBridgePort) {
  auto& bridgeApi = saiApiTable->bridgeApi();
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT,
                                          42,
                                          true,
                                          SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW};
  auto bridgePortId = bridgeApi.create<SaiBridgePortTraits>(c, 0);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiBridgePortTraits>();

  auto got = store.get(SaiBridgePortTraits::Attributes::PortId{42});
  EXPECT_EQ(got->adapterKey(), bridgePortId);
}

TEST_F(SaiStoreTest, loadBridgePortFromJson) {
  auto& bridgeApi = saiApiTable->bridgeApi();
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT,
                                          42,
                                          true,
                                          SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW};
  auto bridgePortId = bridgeApi.create<SaiBridgePortTraits>(c, 0);

  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  SaiStore s2(0);
  s2.reload(&json);
  auto& store = s2.get<SaiBridgePortTraits>();

  auto got = store.get(SaiBridgePortTraits::Attributes::PortId{42});
  EXPECT_EQ(got->adapterKey(), bridgePortId);
}

/*
 * Note, the default bridge id is a pain to get at, so we will skip
 * the bridgeLoadCtor test. That will largely be covered by testing
 * BridgeStore.reload() anyway.
 */

TEST_F(SaiStoreTest, bridgePortLoadCtor) {
  auto& bridgeApi = saiApiTable->bridgeApi();
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT,
                                          42,
                                          true,
                                          SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW};
  auto bridgePortId = bridgeApi.create<SaiBridgePortTraits>(c, 0);
  SaiObject<SaiBridgePortTraits> obj(bridgePortId);
  EXPECT_EQ(obj.adapterKey(), bridgePortId);
  EXPECT_EQ(GET_ATTR(BridgePort, PortId, obj.attributes()), 42);
}

TEST_F(SaiStoreTest, bridgePortCreateCtor) {
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT,
                                          42,
                                          true,
                                          SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW};
  SaiObject<SaiBridgePortTraits> obj({42}, c, 0);
  EXPECT_EQ(GET_ATTR(BridgePort, PortId, obj.attributes()), 42);
}

TEST_F(SaiStoreTest, serDeserBridge) {
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiBridgeTraits>();
  auto got = store.get(std::monostate{});
  verifyAdapterKeySerDeser<SaiBridgeTraits>({got->adapterKey()});
}

TEST_F(SaiStoreTest, toStrBridge) {
  verifyToStr<SaiBridgeTraits>();
}

TEST_F(SaiStoreTest, serDeserBridgePort) {
  auto& bridgeApi = saiApiTable->bridgeApi();
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT,
                                          42,
                                          true,
                                          SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW};
  auto bridgePortId = bridgeApi.create<SaiBridgePortTraits>(c, 0);
  verifyAdapterKeySerDeser<SaiBridgePortTraits>({bridgePortId});
}

TEST_F(SaiStoreTest, toStrBridgePort) {
  auto& bridgeApi = saiApiTable->bridgeApi();
  SaiBridgePortTraits::CreateAttributes c{SAI_BRIDGE_PORT_TYPE_PORT,
                                          42,
                                          true,
                                          SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW};
  std::ignore = bridgeApi.create<SaiBridgePortTraits>(c, 0);
  verifyToStr<SaiBridgePortTraits>();
}
