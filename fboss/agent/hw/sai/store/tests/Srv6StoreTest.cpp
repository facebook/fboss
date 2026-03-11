// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/Srv6Api.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class Srv6StoreTest : public SaiStoreTest {
 public:
  SaiSrv6SidListTraits::CreateAttributes createSrv6SidListAttrs() const {
    SaiSrv6SidListTraits::Attributes::Type type{
        SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED};
    return {type, std::nullopt, std::nullopt};
  }

  SaiSrv6SidListTraits::AdapterHostKey makeAdapterHostKey() const {
    return SaiSrv6SidListTraits::AdapterHostKey{
        SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED,
        std::nullopt,
        RouterInterfaceSaiId{kRifId},
        kNextHopIp};
  }

  static constexpr sai_object_id_t kRifId{42};
  static inline const folly::IPAddress kNextHopIp{"10.0.0.1"};
};

TEST_F(Srv6StoreTest, setObjectPreservesAdapterHostKey) {
  auto key = makeAdapterHostKey();
  auto attrs = createSrv6SidListAttrs();
  auto& store = saiStore->get<SaiSrv6SidListTraits>();
  auto obj = store.setObject(key, attrs);
  EXPECT_EQ(obj->adapterHostKey(), key);
}

TEST_F(Srv6StoreTest, setObjectTwiceReturnsSameObject) {
  auto key = makeAdapterHostKey();
  auto attrs = createSrv6SidListAttrs();
  auto& store = saiStore->get<SaiSrv6SidListTraits>();
  auto obj1 = store.setObject(key, attrs);
  auto obj2 = store.setObject(key, attrs);
  EXPECT_EQ(obj1->adapterKey(), obj2->adapterKey());
  EXPECT_EQ(obj1->adapterHostKey(), obj2->adapterHostKey());
}

TEST_F(Srv6StoreTest, srv6SidListCreateCtor) {
  auto key = makeAdapterHostKey();
  auto attrs = createSrv6SidListAttrs();
  SaiObject<SaiSrv6SidListTraits> obj =
      createObj<SaiSrv6SidListTraits>(key, attrs, 0);
  EXPECT_EQ(obj.adapterHostKey(), key);
  EXPECT_EQ(
      GET_ATTR(Srv6SidList, Type, obj.attributes()),
      SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
}

TEST_F(Srv6StoreTest, serDeserSrv6SidList) {
  auto key = makeAdapterHostKey();
  auto attrs = createSrv6SidListAttrs();
  auto& store = saiStore->get<SaiSrv6SidListTraits>();
  auto obj = store.setObject(key, attrs);
  auto sidListId = obj->adapterKey();
  // Verify adapter key appears in the serialized JSON
  auto json = saiStore->adapterKeysFollyDynamic();
  auto gotKeys = keysForSaiObjStoreFromStoreJson<SaiSrv6SidListTraits>(json);
  EXPECT_EQ(gotKeys.size(), 1);
  EXPECT_EQ(gotKeys[0], sidListId);
}

TEST_F(Srv6StoreTest, toStrSrv6SidList) {
  auto key = makeAdapterHostKey();
  auto attrs = createSrv6SidListAttrs();
  auto& store = saiStore->get<SaiSrv6SidListTraits>();
  store.setObject(key, attrs);
  auto str = fmt::format("{}", store);
  EXPECT_EQ(std::count(str.begin(), str.end(), '\n'), store.size() + 1);
}
