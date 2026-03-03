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

  Srv6SidListSaiId createSrv6SidList() const {
    auto& srv6Api = saiApiTable->srv6Api();
    return srv6Api.create<SaiSrv6SidListTraits>(createSrv6SidListAttrs(), 0);
  }
};

TEST_F(Srv6StoreTest, loadSrv6SidList) {
  auto sidListId = createSrv6SidList();
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiSrv6SidListTraits>();
  auto obj = createObj<SaiSrv6SidListTraits>(sidListId);
  auto k = obj.adapterHostKey();
  auto got = store.get(k);
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), sidListId);
}

TEST_F(Srv6StoreTest, srv6SidListLoadCtor) {
  auto sidListId = createSrv6SidList();
  SaiObject<SaiSrv6SidListTraits> obj =
      createObj<SaiSrv6SidListTraits>(sidListId);
  EXPECT_EQ(obj.adapterKey(), sidListId);
  EXPECT_EQ(
      GET_ATTR(Srv6SidList, Type, obj.attributes()),
      SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
}

TEST_F(Srv6StoreTest, srv6SidListCreateCtor) {
  SaiSrv6SidListTraits::AdapterHostKey k{
      SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED, std::nullopt, std::nullopt};
  SaiObject<SaiSrv6SidListTraits> obj =
      createObj<SaiSrv6SidListTraits>(k, k, 0);
  EXPECT_EQ(
      GET_ATTR(Srv6SidList, Type, obj.attributes()),
      SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
}

TEST_F(Srv6StoreTest, serDeserSrv6SidList) {
  auto sidListId = createSrv6SidList();
  verifyAdapterKeySerDeser<SaiSrv6SidListTraits>({sidListId});
}

TEST_F(Srv6StoreTest, toStrSrv6SidList) {
  std::ignore = createSrv6SidList();
  verifyToStr<SaiSrv6SidListTraits>();
}
