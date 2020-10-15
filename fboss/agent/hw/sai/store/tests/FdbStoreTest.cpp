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

#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

TEST_F(SaiStoreTest, loadFdb) {
  auto& fdbApi = saiApiTable->fdbApi();
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 10, mac);
  fdbApi.create<SaiFdbTraits>(f, {SAI_FDB_ENTRY_TYPE_STATIC, 42, 24});

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiFdbTraits>();

  auto got = store.get(f);
  EXPECT_EQ(got->adapterKey(), f);
  EXPECT_EQ(
      std::get<SaiFdbTraits::Attributes::BridgePortId>(got->attributes())
          .value(),
      42);
  EXPECT_EQ(GET_OPT_ATTR(Fdb, Metadata, got->attributes()), 24);
}

TEST_F(SaiStoreTest, fdbLoadCtor) {
  auto& fdbApi = saiApiTable->fdbApi();
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 10, mac);
  fdbApi.create<SaiFdbTraits>(f, {SAI_FDB_ENTRY_TYPE_STATIC, 42, 24});

  SaiObject<SaiFdbTraits> obj(f);
  EXPECT_EQ(obj.adapterKey(), f);
  EXPECT_EQ(
      std::get<SaiFdbTraits::Attributes::BridgePortId>(obj.attributes())
          .value(),
      42);
  EXPECT_EQ(GET_ATTR(Fdb, BridgePortId, obj.attributes()), 42);
  EXPECT_EQ(GET_OPT_ATTR(Fdb, Metadata, obj.attributes()), 24);
}

TEST_F(SaiStoreTest, fdbCreateCtor) {
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 10, mac);
  SaiObject<SaiFdbTraits> obj(f, {SAI_FDB_ENTRY_TYPE_STATIC, 42, 24}, 0);
  EXPECT_EQ(GET_ATTR(Fdb, BridgePortId, obj.attributes()), 42);
  EXPECT_EQ(GET_OPT_ATTR(Fdb, Metadata, obj.attributes()), 24);
}

TEST_F(SaiStoreTest, fdbSetBridgePort) {
  auto& fdbApi = saiApiTable->fdbApi();
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 10, mac);
  fdbApi.create<SaiFdbTraits>(f, {SAI_FDB_ENTRY_TYPE_STATIC, 42, std::nullopt});

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiFdbTraits>();

  auto obj = store.get(f);
  EXPECT_EQ(GET_ATTR(Fdb, BridgePortId, obj->attributes()), 42);
  store.setObject(
      f,
      {SAI_FDB_ENTRY_TYPE_STATIC, 43, 23},
      PublisherKey<SaiFdbTraits>::type{});
  EXPECT_EQ(GET_ATTR(Fdb, BridgePortId, obj->attributes()), 43);
  EXPECT_EQ(GET_OPT_ATTR(Fdb, Metadata, obj->attributes()), 23);
}

TEST_F(SaiStoreTest, fdbSerDeser) {
  auto& fdbApi = saiApiTable->fdbApi();
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 10, mac);
  fdbApi.create<SaiFdbTraits>(f, {SAI_FDB_ENTRY_TYPE_STATIC, 42, 24});
  verifyAdapterKeySerDeser<SaiFdbTraits>({f});
}

TEST_F(SaiStoreTest, fdbToStr) {
  auto& fdbApi = saiApiTable->fdbApi();
  folly::MacAddress mac{"42:42:42:42:42:42"};
  SaiFdbTraits::FdbEntry f(0, 10, mac);
  fdbApi.create<SaiFdbTraits>(f, {SAI_FDB_ENTRY_TYPE_STATIC, 42, 24});
  verifyToStr<SaiFdbTraits>();
}
