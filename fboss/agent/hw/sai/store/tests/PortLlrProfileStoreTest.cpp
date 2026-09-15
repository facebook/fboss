/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

#if SAI_API_VERSION >= SAI_VERSION(1, 18, 0)
class PortLlrProfileStoreTest : public SaiStoreTest {
 public:
  SaiPortLlrProfileTraits::CreateAttributes makeAttrs() {
    return SaiPortLlrProfileTraits::CreateAttributes{
        SaiPortLlrProfileTraits::Attributes::OutstandingFramesMax{32},
        SaiPortLlrProfileTraits::Attributes::OutstandingBytesMax{4096},
        SaiPortLlrProfileTraits::Attributes::ReplayTimerMax{5000},
        SaiPortLlrProfileTraits::Attributes::ReplayCountMax{sai_uint8_t(7)},
        SaiPortLlrProfileTraits::Attributes::PcsLostTimeout{1000},
        SaiPortLlrProfileTraits::Attributes::DataAgeTimeout{200000},
        SaiPortLlrProfileTraits::Attributes::InitLlrFrameAction{
            SAI_LLR_FRAME_ACTION_BEST_EFFORT},
        SaiPortLlrProfileTraits::Attributes::FlushLlrFrameAction{
            SAI_LLR_FRAME_ACTION_BLOCK},
        SaiPortLlrProfileTraits::Attributes::ReInitOnFlush{true},
        SaiPortLlrProfileTraits::Attributes::CtlosTargetSpacing{
            sai_uint16_t(2048)}};
  }

  PortLlrProfileSaiId createLlrProfile() {
    return saiApiTable->portApi().create<SaiPortLlrProfileTraits>(
        makeAttrs(), 0);
  }
};

// A fresh SaiStore reload reclaims a previously created LLR profile and keys it
// by its (content) AdapterHostKey -- the warm-boot reclaim path exercised on
// every agent restart.
TEST_F(PortLlrProfileStoreTest, loadPortLlrProfile) {
  auto id = createLlrProfile();

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiPortLlrProfileTraits>();

  SaiPortLlrProfileTraits::AdapterHostKey k = makeAttrs();
  auto got = store.get(k);
  ASSERT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), id);
}

// Reload from serialized adapter keys (warm boot restores the store from the
// persisted JSON rather than a live adapter scan).
TEST_F(PortLlrProfileStoreTest, loadPortLlrProfileFromJson) {
  auto id = createLlrProfile();

  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  SaiStore s2(0);
  s2.reload(&json);
  auto& store = s2.get<SaiPortLlrProfileTraits>();

  SaiPortLlrProfileTraits::AdapterHostKey k = makeAttrs();
  auto got = store.get(k);
  ASSERT_NE(got, nullptr);
  EXPECT_EQ(got->adapterKey(), id);
}

TEST_F(PortLlrProfileStoreTest, portLlrProfileLoadCtor) {
  auto id = createLlrProfile();
  auto obj = createObj<SaiPortLlrProfileTraits>(id);
  EXPECT_EQ(obj.adapterKey(), id);
  EXPECT_EQ(GET_ATTR(PortLlrProfile, ReplayCountMax, obj.attributes()), 7);
}

TEST_F(PortLlrProfileStoreTest, portLlrProfileSerDeser) {
  auto id = createLlrProfile();
  verifyAdapterKeySerDeser<SaiPortLlrProfileTraits>({id});
}

TEST_F(PortLlrProfileStoreTest, portLlrProfileToStr) {
  std::ignore = createLlrProfile();
  verifyToStr<SaiPortLlrProfileTraits>();
}
#endif
