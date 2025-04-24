// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

using namespace ::testing;

#include "fboss/agent/hw/sai/api/MplsApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

TEST_F(SaiStoreTest, createInSegEntry) {
  auto& mplsApi = saiApiTable->mplsApi();
  typename facebook::fboss::SaiInSegTraits::InSegEntry entry{0, 100};
  typename facebook::fboss::SaiInSegTraits::Attributes::NextHopId
      nextHopIdAttribute(1010);
  typename facebook::fboss::SaiInSegTraits::CreateAttributes attributes{
      SAI_PACKET_ACTION_FORWARD, 1, nextHopIdAttribute};
  mplsApi.create<facebook::fboss::SaiInSegTraits>(entry, attributes);

  saiStore->reload();
  auto& store = saiStore->get<facebook::fboss::SaiInSegTraits>();

  auto got = store.get(entry);
  EXPECT_EQ(got->adapterKey(), entry);
  EXPECT_EQ(
      GET_ATTR(InSeg, PacketAction, got->attributes()),
      SAI_PACKET_ACTION_FORWARD);
  EXPECT_EQ(GET_ATTR(InSeg, NumOfPop, got->attributes()), 1);
  EXPECT_EQ(GET_OPT_ATTR(InSeg, NextHopId, got->attributes()), 1010);
}

TEST_F(SaiStoreTest, modifyInSegEntry) {
  auto& mplsApi = saiApiTable->mplsApi();
  typename facebook::fboss::SaiInSegTraits::InSegEntry entry{0, 100};
  typename facebook::fboss::SaiInSegTraits::Attributes::NextHopId
      nextHopIdAttribute(1010);
  typename SaiInSegTraits::CreateAttributes attributes{
      SAI_PACKET_ACTION_FORWARD, 1, nextHopIdAttribute};
  mplsApi.create<SaiInSegTraits>(entry, attributes);
  mplsApi.setAttribute(entry, SaiInSegTraits::Attributes::NextHopId{1011});

  saiStore->reload();
  auto& store = saiStore->get<SaiInSegTraits>();

  auto got = store.get(entry);
  EXPECT_EQ(got->adapterKey(), entry);
  EXPECT_EQ(
      GET_ATTR(InSeg, PacketAction, got->attributes()),
      SAI_PACKET_ACTION_FORWARD);
  EXPECT_EQ(GET_ATTR(InSeg, NumOfPop, got->attributes()), 1);
  EXPECT_EQ(GET_OPT_ATTR(InSeg, NextHopId, got->attributes()), 1011);
}

TEST_F(SaiStoreTest, InsegEntrySerDeser) {
  auto& mplsApi = saiApiTable->mplsApi();
  typename SaiInSegTraits::InSegEntry entry{0, 100};
  typename SaiInSegTraits::Attributes::NextHopId nextHopIdAttribute(1010);
  typename SaiInSegTraits::CreateAttributes attributes{
      SAI_PACKET_ACTION_FORWARD, 1, nextHopIdAttribute};
  mplsApi.create<SaiInSegTraits>(entry, attributes);
  verifyAdapterKeySerDeser<SaiInSegTraits>({entry});
}

TEST_F(SaiStoreTest, InsegEntryToStr) {
  auto& mplsApi = saiApiTable->mplsApi();
  typename SaiInSegTraits::InSegEntry entry{0, 100};
  typename SaiInSegTraits::Attributes::NextHopId nextHopIdAttribute(1010);
  typename SaiInSegTraits::CreateAttributes attributes{
      SAI_PACKET_ACTION_FORWARD, 1, nextHopIdAttribute};
  mplsApi.create<SaiInSegTraits>(entry, attributes);
  verifyToStr<SaiInSegTraits>();
}
