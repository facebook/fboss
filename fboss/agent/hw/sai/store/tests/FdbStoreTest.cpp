/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class FdbStoreTest : public ::testing::Test {
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

TEST_F(FdbStoreTest, loadFdb) {
  auto& fdbApi = saiApiTable->fdbApi();
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 10, mac);
  fdbApi.create2<SaiFdbTraits>(f, {SAI_FDB_ENTRY_TYPE_STATIC, 42});

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiFdbTraits>();

  auto got = store.get(f);
  EXPECT_EQ(got->adapterKey(), f);
  EXPECT_EQ(
      std::get<SaiFdbTraits::Attributes::BridgePortId>(got->attributes())
          .value(),
      42);
}

TEST_F(FdbStoreTest, fdbLoadCtor) {
  auto& fdbApi = saiApiTable->fdbApi();
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 10, mac);
  fdbApi.create2<SaiFdbTraits>(f, {SAI_FDB_ENTRY_TYPE_STATIC, 42});

  SaiObject<SaiFdbTraits> obj(f);
  EXPECT_EQ(obj.adapterKey(), f);
  EXPECT_EQ(
      std::get<SaiFdbTraits::Attributes::BridgePortId>(obj.attributes())
          .value(),
      42);
  EXPECT_EQ(GET_ATTR(Fdb, BridgePortId, obj.attributes()), 42);
}

TEST_F(FdbStoreTest, fdbCreateCtor) {
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 10, mac);
  SaiObject<SaiFdbTraits> obj(f, {SAI_FDB_ENTRY_TYPE_STATIC, 42}, 0);
  EXPECT_EQ(GET_ATTR(Fdb, BridgePortId, obj.attributes()), 42);
}

TEST_F(FdbStoreTest, fdbSetBridgePort) {
  auto& fdbApi = saiApiTable->fdbApi();
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 10, mac);
  fdbApi.create2<SaiFdbTraits>(f, {SAI_FDB_ENTRY_TYPE_STATIC, 42});

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiFdbTraits>();

  auto obj = store.get(f);
  EXPECT_EQ(GET_ATTR(Fdb, BridgePortId, obj->attributes()), 42);
  store.setObject(f, {SAI_FDB_ENTRY_TYPE_STATIC, 43});
  EXPECT_EQ(GET_ATTR(Fdb, BridgePortId, obj->attributes()), 43);
}
