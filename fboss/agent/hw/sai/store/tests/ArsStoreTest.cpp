/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/ArsApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class ArsStoreTest : public SaiStoreTest {
 public:
  ArsSaiId createArs(sai_uint32_t idleTime, sai_uint32_t maxFlows) {
    return saiApiTable->arsApi().create<SaiArsTraits>(
        {SAI_ARS_MODE_FLOWLET_QUALITY, idleTime, maxFlows}, 0);
  }
};

TEST_F(ArsStoreTest, loadArs) {
  auto arsSaiId1 = createArs(20000, 2000);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiArsTraits>();

  auto got = store.get(std::monostate{});
  EXPECT_EQ(got->adapterKey(), arsSaiId1);
}

TEST_F(ArsStoreTest, arsLoadCtor) {
  auto arsSaiId = createArs(20000, 2000);
  auto obj = createObj<SaiArsTraits>(arsSaiId);
  EXPECT_EQ(obj.adapterKey(), arsSaiId);
  EXPECT_EQ(GET_ATTR(Ars, IdleTime, obj.attributes()), 20000);
  EXPECT_EQ(GET_ATTR(Ars, MaxFlows, obj.attributes()), 2000);
}

TEST_F(ArsStoreTest, arsCreateCtor) {
  SaiArsTraits::CreateAttributes c{SAI_ARS_MODE_FLOWLET_QUALITY, 40000, 4000};
  auto obj = createObj<SaiArsTraits>(std::monostate{}, c, 0);
  EXPECT_EQ(GET_ATTR(Ars, IdleTime, obj.attributes()), 40000);
  EXPECT_EQ(GET_ATTR(Ars, MaxFlows, obj.attributes()), 4000);
}

TEST_F(ArsStoreTest, arsSerDeser) {
  auto arsSaiId = createArs(30000, 3000);
  verifyAdapterKeySerDeser<SaiArsTraits>({arsSaiId});
}

TEST_F(ArsStoreTest, arsToStr) {
  std::ignore = createArs(50000, 5000);
  verifyToStr<SaiArsTraits>();
}
