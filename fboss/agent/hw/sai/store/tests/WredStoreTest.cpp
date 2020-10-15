/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/WredApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class WredStoreTest : public SaiStoreTest {
 public:
  WredSaiId createWredProfile(
      const bool greenEnable,
      const uint32_t greenMinThreshold,
      const uint32_t greenMaxThreshold,
      const int32_t ecnMarkMode,
      const uint32_t ecnGreenMinThreshold,
      const uint32_t ecnGreenMaxThreshold) const {
    return saiApiTable->wredApi().create<SaiWredTraits>(
        {greenEnable,
         greenMinThreshold,
         greenMaxThreshold,
         ecnMarkMode,
         ecnGreenMinThreshold,
         ecnGreenMaxThreshold},
        0);
  }

  SaiWredTraits::AdapterHostKey createWredProfileAdapterHostKey(
      const bool greenEnable,
      const uint32_t greenMinThreshold,
      const uint32_t greenMaxThreshold,
      const int32_t ecnMarkMode,
      const uint32_t ecnGreenMinThreshold,
      const uint32_t ecnGreenMaxThreshold) const {
    SaiWredTraits::Attributes::GreenEnable greenEnableAttribute{greenEnable};
    SaiWredTraits::Attributes::GreenMinThreshold greenMinThresholdAttribute{
        greenMinThreshold};
    SaiWredTraits::Attributes::GreenMaxThreshold greenMaxThresholdAttribute{
        greenMaxThreshold};
    SaiWredTraits::Attributes::EcnMarkMode ecnMarkModeAttribute{ecnMarkMode};
    SaiWredTraits::Attributes::EcnGreenMinThreshold
        ecnGreenMinThresholdAttribute{ecnGreenMinThreshold};
    SaiWredTraits::Attributes::EcnGreenMaxThreshold
        ecnGreenMaxThresholdAttribute{ecnGreenMaxThreshold};
    SaiWredTraits::AdapterHostKey k{greenEnableAttribute,
                                    greenMinThresholdAttribute,
                                    greenMaxThresholdAttribute,
                                    ecnMarkModeAttribute,
                                    ecnGreenMinThresholdAttribute,
                                    ecnGreenMaxThresholdAttribute};
    return k;
  }
};

TEST_F(WredStoreTest, loadWredProfile) {
  auto wredId0 = createWredProfile(true, 100, 200, 0, 0, 0);
  auto wredId1 = createWredProfile(true, 300, 400, 0, 0, 0);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiWredTraits>();

  SaiWredTraits::AdapterHostKey k0 =
      createWredProfileAdapterHostKey(true, 100, 200, 0, 0, 0);
  SaiWredTraits::AdapterHostKey k1 =
      createWredProfileAdapterHostKey(true, 300, 400, 0, 0, 0);

  auto got0 = store.get(k0);
  EXPECT_EQ(got0->adapterKey(), wredId0);
  auto got1 = store.get(k1);
  EXPECT_EQ(got1->adapterKey(), wredId1);
}

TEST_F(WredStoreTest, loadEcnProfile) {
  auto ecnId0 = createWredProfile(false, 0, 0, 0, 100, 200);
  auto ecnId1 = createWredProfile(false, 0, 0, 1, 300, 400);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiWredTraits>();

  SaiWredTraits::AdapterHostKey k0 =
      createWredProfileAdapterHostKey(false, 0, 0, 0, 100, 200);
  SaiWredTraits::AdapterHostKey k1 =
      createWredProfileAdapterHostKey(false, 0, 0, 1, 300, 400);

  auto got0 = store.get(k0);
  EXPECT_EQ(got0->adapterKey(), ecnId0);
  auto got1 = store.get(k1);
  EXPECT_EQ(got1->adapterKey(), ecnId1);
}

TEST_F(WredStoreTest, wredProfileLoadCtor) {
  auto wredId = createWredProfile(true, 100, 200, 0, 0, 0);
  SaiObject<SaiWredTraits> obj(wredId);
  EXPECT_EQ(obj.adapterKey(), wredId);
}

TEST_F(WredStoreTest, ecnProfileLoadCtor) {
  auto ecnId = createWredProfile(false, 0, 0, 1, 100, 200);
  SaiObject<SaiWredTraits> obj(ecnId);
  EXPECT_EQ(obj.adapterKey(), ecnId);
}

TEST_F(WredStoreTest, wredProfileCreateCtor) {
  SaiWredTraits::CreateAttributes c{true, 100, 200, 0, 0, 0};
  SaiWredTraits::AdapterHostKey k =
      createWredProfileAdapterHostKey(true, 100, 200, 0, 0, 0);
  SaiObject<SaiWredTraits> obj(k, c, 0);
  EXPECT_TRUE(GET_ATTR(Wred, GreenEnable, obj.attributes()));
  EXPECT_EQ(GET_OPT_ATTR(Wred, GreenMinThreshold, obj.attributes()), 100);
  EXPECT_EQ(GET_OPT_ATTR(Wred, GreenMaxThreshold, obj.attributes()), 200);
  EXPECT_EQ(GET_ATTR(Wred, EcnMarkMode, obj.attributes()), 0);
  EXPECT_EQ(GET_OPT_ATTR(Wred, EcnGreenMinThreshold, obj.attributes()), 0);
  EXPECT_EQ(GET_OPT_ATTR(Wred, EcnGreenMaxThreshold, obj.attributes()), 0);
}

TEST_F(WredStoreTest, ecnProfileCreateCtor) {
  SaiWredTraits::CreateAttributes c{false, 0, 0, 1, 300, 400};
  SaiWredTraits::AdapterHostKey k =
      createWredProfileAdapterHostKey(false, 0, 0, 1, 300, 400);
  SaiObject<SaiWredTraits> obj(k, c, 0);
  EXPECT_FALSE(GET_ATTR(Wred, GreenEnable, obj.attributes()));
  EXPECT_EQ(GET_OPT_ATTR(Wred, GreenMinThreshold, obj.attributes()), 0);
  EXPECT_EQ(GET_OPT_ATTR(Wred, GreenMaxThreshold, obj.attributes()), 0);
  EXPECT_EQ(GET_ATTR(Wred, EcnMarkMode, obj.attributes()), 1);
  EXPECT_EQ(GET_OPT_ATTR(Wred, EcnGreenMinThreshold, obj.attributes()), 300);
  EXPECT_EQ(GET_OPT_ATTR(Wred, EcnGreenMaxThreshold, obj.attributes()), 400);
}

TEST_F(WredStoreTest, serDeserWredProfileStore) {
  auto wredId = createWredProfile(true, 100, 200, 0, 0, 0);
  verifyAdapterKeySerDeser<SaiWredTraits>({wredId});
}

TEST_F(WredStoreTest, serDeserEcnProfileStore) {
  auto ecnId = createWredProfile(false, 0, 0, 1, 300, 400);
  verifyAdapterKeySerDeser<SaiWredTraits>({ecnId});
}

TEST_F(WredStoreTest, toStrWredProfileStore) {
  std::ignore = createWredProfile(true, 100, 200, 0, 0, 0);
  verifyToStr<SaiWredTraits>();
}

TEST_F(WredStoreTest, toStrEcnProfileStore) {
  std::ignore = createWredProfile(false, 0, 0, 1, 300, 400);
  verifyToStr<SaiWredTraits>();
}
