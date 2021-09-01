/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/SamplePacketApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <variant>

using namespace facebook::fboss;

class SamplePacketStoreTest : public SaiStoreTest {
 public:
  SamplePacketSaiId createSamplePacket(
      uint16_t rate,
      sai_samplepacket_type_t type,
      sai_samplepacket_mode_t mode) {
    SaiSamplePacketTraits::CreateAttributes c{rate, type, mode};
    return saiApiTable->samplePacketApi().create<SaiSamplePacketTraits>(c, 0);
  }
};

TEST_F(SamplePacketStoreTest, loadSamplePacket) {
  auto samplePacket1 = createSamplePacket(
      20, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_EXCLUSIVE);
  auto samplePacket2 = createSamplePacket(
      30, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_SHARED);

  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiSamplePacketTraits>();

  SaiSamplePacketTraits::AdapterHostKey k1{
      20, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_EXCLUSIVE};
  SaiSamplePacketTraits::AdapterHostKey k2{
      30, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_SHARED};
  auto got = store.get(k1);
  EXPECT_EQ(got->adapterKey(), samplePacket1);
  got = store.get(k2);
  EXPECT_EQ(got->adapterKey(), samplePacket2);
}

TEST_F(SamplePacketStoreTest, samplePacketLoadCtor) {
  auto samplePacketId = createSamplePacket(
      30, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_SHARED);
  auto obj = createObj<SaiSamplePacketTraits>(samplePacketId);
  EXPECT_EQ(obj.adapterKey(), samplePacketId);
  EXPECT_EQ(
      GET_ATTR(SamplePacket, Type, obj.attributes()),
      SAI_SAMPLEPACKET_TYPE_SLOW_PATH);
}

TEST_F(SamplePacketStoreTest, samplePacketCreateCtor) {
  SaiSamplePacketTraits::AdapterHostKey k{
      30, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_SHARED};
  SaiSamplePacketTraits::CreateAttributes c{
      30, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_SHARED};
  auto obj = createObj<SaiSamplePacketTraits>(k, c, 0);
  EXPECT_EQ(
      GET_ATTR(SamplePacket, Type, obj.attributes()),
      SAI_SAMPLEPACKET_TYPE_SLOW_PATH);
}

TEST_F(SamplePacketStoreTest, serDeser) {
  auto id = createSamplePacket(
      20, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_EXCLUSIVE);
  verifyAdapterKeySerDeser<SaiSamplePacketTraits>({id});
}

TEST_F(SamplePacketStoreTest, toStr) {
  std::ignore = createSamplePacket(
      20, SAI_SAMPLEPACKET_TYPE_SLOW_PATH, SAI_SAMPLEPACKET_MODE_EXCLUSIVE);
  verifyToStr<SaiSamplePacketTraits>();
}
