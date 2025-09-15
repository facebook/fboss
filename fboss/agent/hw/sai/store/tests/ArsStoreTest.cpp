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
  ArsSaiId createArs(
      sai_uint32_t idleTime,
      sai_uint32_t maxFlows,
      sai_uint32_t primaryPathQualityThreshold,
      sai_uint32_t alternatePathCost,
      sai_uint32_t alternatePathBias) {
    return saiApiTable->arsApi().create<SaiArsTraits>(
        {SaiArsTraits::Attributes::Mode{SAI_ARS_MODE_FLOWLET_QUALITY},
         SaiArsTraits::Attributes::IdleTime{idleTime},
         SaiArsTraits::Attributes::MaxFlows{maxFlows},
         primaryPathQualityThreshold,
         alternatePathCost,
         alternatePathBias},
        0);
  }
};

TEST_F(ArsStoreTest, loadArs) {
  auto arsSaiId1 = createArs(20000, 2000, 100, 200, 50);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiArsTraits>();

  auto got = store.get(std::monostate{});
  EXPECT_EQ(got->adapterKey(), arsSaiId1);
}

TEST_F(ArsStoreTest, arsLoadCtor) {
  auto arsSaiId = createArs(20000, 2000, 120, 250, 60);
  auto obj = createObj<SaiArsTraits>(arsSaiId);
  EXPECT_EQ(obj.adapterKey(), arsSaiId);
  EXPECT_EQ(GET_OPT_ATTR(Ars, IdleTime, obj.attributes()), 20000);
  EXPECT_EQ(GET_OPT_ATTR(Ars, MaxFlows, obj.attributes()), 2000);
  EXPECT_EQ(
      GET_OPT_ATTR(Ars, PrimaryPathQualityThreshold, obj.attributes()), 120);
  EXPECT_EQ(GET_OPT_ATTR(Ars, AlternatePathCost, obj.attributes()), 250);
  EXPECT_EQ(GET_OPT_ATTR(Ars, AlternatePathBias, obj.attributes()), 60);
}

TEST_F(ArsStoreTest, arsCreateCtor) {
  SaiArsTraits::CreateAttributes c{
      SaiArsTraits::Attributes::Mode{SAI_ARS_MODE_FLOWLET_QUALITY},
      SaiArsTraits::Attributes::IdleTime{40000},
      SaiArsTraits::Attributes::MaxFlows{4000},
      150, // Primary path quality threshold
      300, // Alternate path cost
      75}; // Alternate path bias
  auto obj = createObj<SaiArsTraits>(std::monostate{}, c, 0);
  EXPECT_EQ(GET_OPT_ATTR(Ars, IdleTime, obj.attributes()), 40000);
  EXPECT_EQ(GET_OPT_ATTR(Ars, MaxFlows, obj.attributes()), 4000);
  EXPECT_EQ(
      GET_OPT_ATTR(Ars, PrimaryPathQualityThreshold, obj.attributes()), 150);
  EXPECT_EQ(GET_OPT_ATTR(Ars, AlternatePathCost, obj.attributes()), 300);
  EXPECT_EQ(GET_OPT_ATTR(Ars, AlternatePathBias, obj.attributes()), 75);
}

TEST_F(ArsStoreTest, arsSerDeser) {
  auto arsSaiId = createArs(30000, 3000, 130, 260, 65);
  verifyAdapterKeySerDeser<SaiArsTraits>({arsSaiId});
}

TEST_F(ArsStoreTest, arsToStr) {
  std::ignore = createArs(50000, 5000, 140, 280, 70);
  verifyToStr<SaiArsTraits>();
}
