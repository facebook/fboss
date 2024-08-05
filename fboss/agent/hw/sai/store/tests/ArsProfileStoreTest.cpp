/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/ArsProfileApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class ArsProfileStoreTest : public SaiStoreTest {
 public:
  ArsProfileSaiId createArsProfile() {
    return saiApiTable->arsProfileApi().create<SaiArsProfileTraits>(
        {SAI_ARS_PROFILE_ALGO_EWMA,
         20000,
         0x5555,
         false,
         false,
         true,
         60,
         600,
         6000,
         true,
         20,
         200,
         2000,
         true,
         20,
         500,
         5000},
        0);
  }
};

TEST_F(ArsProfileStoreTest, loadArsProfile) {
  auto arsProfileSaiId = createArsProfile();

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiArsProfileTraits>();
  auto got = store.get(std::monostate{});
  EXPECT_EQ(got->adapterKey(), arsProfileSaiId);
}

TEST_F(ArsProfileStoreTest, arsProfileLoadCtor) {
  auto arsProfileSaiId = createArsProfile();
  auto obj = createObj<SaiArsProfileTraits>(arsProfileSaiId);
  EXPECT_EQ(obj.adapterKey(), arsProfileSaiId);
  EXPECT_EQ(GET_ATTR(ArsProfile, SamplingInterval, obj.attributes()), 20000);
}

TEST_F(ArsProfileStoreTest, arsProfileCreateCtor) {
  SaiArsProfileTraits::CreateAttributes c{
      SAI_ARS_PROFILE_ALGO_EWMA,
      20000,
      0x5555,
      false,
      false,
      true,
      60,
      600,
      6000,
      true,
      20,
      200,
      2000,
      true,
      20,
      500,
      5000};
  auto obj = createObj<SaiArsProfileTraits>(std::monostate{}, c, 0);
  EXPECT_EQ(GET_ATTR(ArsProfile, SamplingInterval, obj.attributes()), 20000);
}

TEST_F(ArsProfileStoreTest, arsProfileSerDeser) {
  auto arsProfileSaiId = createArsProfile();
  verifyAdapterKeySerDeser<SaiArsProfileTraits>({arsProfileSaiId});
}

TEST_F(ArsProfileStoreTest, arsProfileToStr) {
  std::ignore = createArsProfile();
  verifyToStr<SaiArsProfileTraits>();
}
