/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/BufferApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <variant>

using namespace facebook::fboss;

class BufferStoreTest : public SaiStoreTest {
 public:
  SaiBufferPoolTraits::CreateAttributes createPoolAttrs() const {
    SaiBufferPoolTraits::Attributes::Type type{SAI_BUFFER_POOL_TYPE_EGRESS};
    SaiBufferPoolTraits::Attributes::Size size{42};
    SaiBufferPoolTraits::Attributes::ThresholdMode mode{
        SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC};
    std::optional<SaiBufferPoolTraits::Attributes::XoffSize> xoffSize{0};
    return {type, size, mode, xoffSize};
  }

  BufferPoolSaiId createBufferPool() const {
    auto& bufferApi = saiApiTable->bufferApi();
    return bufferApi.create<SaiBufferPoolTraits>(createPoolAttrs(), 0);
  }

  template <typename BufferProfileTraits>
  BufferProfileTraits::CreateAttributes createProfileAttrs(
      BufferPoolSaiId _pool) const {
    SaiBufferProfileAttributes::PoolId pool{_pool};
    std::optional<SaiBufferProfileAttributes::ReservedBytes> reservedBytes{42};
    std::optional<SaiBufferProfileAttributes::ThresholdMode> mode;
    std::optional<SaiBufferProfileAttributes::XoffTh> xoffTh{293624};
    std::optional<SaiBufferProfileAttributes::XonTh> xonTh{0};
    std::optional<SaiBufferProfileAttributes::XonOffsetTh> xonOffsetTh{4826};
    if constexpr (std::is_same_v<
                      BufferProfileTraits,
                      SaiDynamicBufferProfileTraits>) {
      mode = SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC;
      std::optional<
          SaiDynamicBufferProfileTraits::Attributes::SharedDynamicThreshold>
          dynamicThresh{24};
      return typename SaiDynamicBufferProfileTraits::CreateAttributes{
          pool,
          reservedBytes,
          mode,
          dynamicThresh,
          xoffTh,
          xonTh,
          xonOffsetTh,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt};
    } else {
      mode = SAI_BUFFER_PROFILE_THRESHOLD_MODE_STATIC;
      std::optional<
          SaiStaticBufferProfileTraits::Attributes::SharedStaticThreshold>
          staticThresh{34};
      return typename SaiStaticBufferProfileTraits::CreateAttributes{
          pool,
          reservedBytes,
          mode,
          staticThresh,
          xoffTh,
          xonTh,
          xonOffsetTh,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt};
    }
  }

  template <typename BufferProfileTraits>
  BufferProfileSaiId createBufferProfile(BufferPoolSaiId _pool) {
    auto& bufferApi = saiApiTable->bufferApi();
    return bufferApi.create<BufferProfileTraits>(
        createProfileAttrs<BufferProfileTraits>(_pool), 0);
  }

  SaiIngressPriorityGroupTraits::CreateAttributes
  createIngressPriorityGroupAttrs(BufferProfileSaiId profileId) const {
    SaiIngressPriorityGroupTraits::Attributes::Port port{1};
    SaiIngressPriorityGroupTraits::Attributes::Index index{10};
    std::optional<SaiIngressPriorityGroupTraits::Attributes::BufferProfile>
        bufferProfile{profileId};
    std::optional<SaiIngressPriorityGroupTraits::Attributes::LosslessEnable>
        losslessEnable;

    return SaiIngressPriorityGroupTraits::CreateAttributes{
        port, index, bufferProfile, losslessEnable};
  }

  IngressPriorityGroupSaiId createIngressPriorityGroup(
      BufferProfileSaiId profileId) const {
    auto& bufferApi = saiApiTable->bufferApi();
    return bufferApi.create<SaiIngressPriorityGroupTraits>(
        createIngressPriorityGroupAttrs(profileId), 0);
  }
};

TEST_F(BufferStoreTest, loadBufferPool) {
  auto poolId = createBufferPool();
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiBufferPoolTraits>();
  SaiBufferPoolTraits::AdapterHostKey k = tupleProjection<
      SaiBufferPoolTraits::CreateAttributes,
      SaiBufferPoolTraits::AdapterHostKey>(createPoolAttrs());
  auto got = store.get(k);

  EXPECT_EQ(got->adapterKey(), poolId);
  EXPECT_EQ(
      std::get<SaiBufferPoolTraits::Attributes::ThresholdMode>(
          got->attributes()),
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
}

TEST_F(BufferStoreTest, loadBufferProfile) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiDynamicBufferProfileTraits>();
  auto got =
      store.get(createProfileAttrs<SaiDynamicBufferProfileTraits>(poolId));
  EXPECT_EQ(got->adapterKey(), profileId);
  EXPECT_EQ(
      GET_OPT_ATTR(DynamicBufferProfile, ThresholdMode, got->attributes()),
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC);
}

TEST_F(BufferStoreTest, loadStaticBufferProfile) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiStaticBufferProfileTraits>(poolId);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiStaticBufferProfileTraits>();
  auto got =
      store.get(createProfileAttrs<SaiStaticBufferProfileTraits>(poolId));
  EXPECT_EQ(got->adapterKey(), profileId);
  EXPECT_EQ(
      GET_OPT_ATTR(StaticBufferProfile, ThresholdMode, got->attributes()),
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_STATIC);
}

TEST_F(BufferStoreTest, loadBufferPoolFromJson) {
  auto poolId = createBufferPool();
  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  SaiStore s2(0);
  s2.reload(&json);
  auto& store = s2.get<SaiBufferPoolTraits>();
  SaiBufferPoolTraits::AdapterHostKey k = tupleProjection<
      SaiBufferPoolTraits::CreateAttributes,
      SaiBufferPoolTraits::AdapterHostKey>(createPoolAttrs());
  auto got = store.get(k);
  EXPECT_EQ(got->adapterKey(), poolId);
  EXPECT_EQ(
      std::get<SaiBufferPoolTraits::Attributes::ThresholdMode>(
          got->attributes()),
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
}

TEST_F(BufferStoreTest, loadBufferProfileFromJson) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  SaiStore s2(0);
  s2.reload(&json);
  auto& store = s2.get<SaiDynamicBufferProfileTraits>();
  auto got =
      store.get(createProfileAttrs<SaiDynamicBufferProfileTraits>(poolId));
  EXPECT_EQ(got->adapterKey(), profileId);
  EXPECT_EQ(
      GET_OPT_ATTR(DynamicBufferProfile, ThresholdMode, got->attributes()),
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC);
}

TEST_F(BufferStoreTest, loadStaticBufferProfileFromJson) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiStaticBufferProfileTraits>(poolId);
  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  SaiStore s2(0);
  s2.reload(&json);
  auto& store = s2.get<SaiStaticBufferProfileTraits>();
  auto got =
      store.get(createProfileAttrs<SaiStaticBufferProfileTraits>(poolId));
  EXPECT_EQ(got->adapterKey(), profileId);
  EXPECT_EQ(
      GET_OPT_ATTR(StaticBufferProfile, ThresholdMode, got->attributes()),
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_STATIC);
}

TEST_F(BufferStoreTest, bufferPoolLoadCtor) {
  auto poolId = createBufferPool();
  SaiObject<SaiBufferPoolTraits> obj = createObj<SaiBufferPoolTraits>(poolId);
  EXPECT_EQ(obj.adapterKey(), poolId);
  EXPECT_EQ(GET_ATTR(BufferPool, Size, obj.attributes()), 42);
}

TEST_F(BufferStoreTest, bufferProfileLoadCtor) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  SaiObject<SaiDynamicBufferProfileTraits> obj =
      createObj<SaiDynamicBufferProfileTraits>(profileId);
  EXPECT_EQ(obj.adapterKey(), profileId);
  EXPECT_EQ(
      GET_OPT_ATTR(DynamicBufferProfile, ReservedBytes, obj.attributes()), 42);
}

TEST_F(BufferStoreTest, staticBufferProfileLoadCtor) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiStaticBufferProfileTraits>(poolId);
  SaiObject<SaiStaticBufferProfileTraits> obj =
      createObj<SaiStaticBufferProfileTraits>(profileId);
  EXPECT_EQ(obj.adapterKey(), profileId);
  EXPECT_EQ(
      GET_OPT_ATTR(StaticBufferProfile, ReservedBytes, obj.attributes()), 42);
}

TEST_F(BufferStoreTest, bufferPoolCreateCtor) {
  auto c = createPoolAttrs();
  SaiBufferPoolTraits::AdapterHostKey k = tupleProjection<
      SaiBufferPoolTraits::CreateAttributes,
      SaiBufferPoolTraits::AdapterHostKey>(c);
  SaiObject<SaiBufferPoolTraits> obj = createObj<SaiBufferPoolTraits>(k, c, 0);
  EXPECT_EQ(GET_ATTR(BufferPool, Size, obj.attributes()), 42);
}

TEST_F(BufferStoreTest, bufferProfileCreateCtor) {
  auto c =
      createProfileAttrs<SaiDynamicBufferProfileTraits>(createBufferPool());
  SaiObject<SaiDynamicBufferProfileTraits> obj =
      createObj<SaiDynamicBufferProfileTraits>(c, c, 0);
  EXPECT_EQ(
      GET_OPT_ATTR(DynamicBufferProfile, ReservedBytes, obj.attributes()), 42);
}

TEST_F(BufferStoreTest, staticBufferProfileCreateCtor) {
  auto c = createProfileAttrs<SaiStaticBufferProfileTraits>(createBufferPool());
  SaiObject<SaiStaticBufferProfileTraits> obj =
      createObj<SaiStaticBufferProfileTraits>(c, c, 0);
  EXPECT_EQ(
      GET_OPT_ATTR(StaticBufferProfile, ReservedBytes, obj.attributes()), 42);
}

TEST_F(BufferStoreTest, serDeserBufferPool) {
  auto poolId = createBufferPool();
  verifyAdapterKeySerDeser<SaiBufferPoolTraits>({poolId});
}

TEST_F(BufferStoreTest, toStrBufferPool) {
  std::ignore = createBufferPool();
  verifyToStr<SaiBufferPoolTraits>();
}

TEST_F(BufferStoreTest, serDeserBufferProfile) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  verifyAdapterKeySerDeser<SaiDynamicBufferProfileTraits>({profileId});
}

TEST_F(BufferStoreTest, toStrBufferProfile) {
  auto poolId = createBufferPool();
  std::ignore = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  verifyToStr<SaiDynamicBufferProfileTraits>();
}

TEST_F(BufferStoreTest, loadIngressPriorityGroup) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  auto ingressPriorityGroupId = createIngressPriorityGroup(profileId);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiIngressPriorityGroupTraits>();
  auto got = store.get(createIngressPriorityGroupAttrs(profileId));
  EXPECT_EQ(got->adapterKey(), ingressPriorityGroupId);
  EXPECT_EQ(
      std::get<SaiIngressPriorityGroupTraits::Attributes::Port>(
          got->attributes()),
      1);
  EXPECT_EQ(
      std::get<SaiIngressPriorityGroupTraits::Attributes::Index>(
          got->attributes()),
      10);
}

TEST_F(BufferStoreTest, loadIngressPriorityGroupFromJson) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  auto ingressPriorityGroupId = createIngressPriorityGroup(profileId);
  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  SaiStore s2(0);
  s2.reload(&json);
  auto& store = s2.get<SaiIngressPriorityGroupTraits>();
  auto got = store.get(createIngressPriorityGroupAttrs(profileId));
  EXPECT_EQ(got->adapterKey(), ingressPriorityGroupId);
  EXPECT_EQ(
      std::get<SaiIngressPriorityGroupTraits::Attributes::Port>(
          got->attributes()),
      1);
  EXPECT_EQ(
      std::get<SaiIngressPriorityGroupTraits::Attributes::Index>(
          got->attributes()),
      10);
}

TEST_F(BufferStoreTest, ingressPriorityGroupLoadCtor) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  auto ingressPriorityGroupId = createIngressPriorityGroup(profileId);
  SaiObject<SaiIngressPriorityGroupTraits> obj =
      createObj<SaiIngressPriorityGroupTraits>(ingressPriorityGroupId);
  EXPECT_EQ(obj.adapterKey(), ingressPriorityGroupId);
  EXPECT_EQ(GET_ATTR(IngressPriorityGroup, Index, obj.attributes()), 10);
}

TEST_F(BufferStoreTest, ingressPriorityGroupCreateCtor) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  auto c = createIngressPriorityGroupAttrs(profileId);
  SaiObject<SaiIngressPriorityGroupTraits> obj =
      createObj<SaiIngressPriorityGroupTraits>(c, c, 0);
  EXPECT_EQ(GET_ATTR(IngressPriorityGroup, Index, obj.attributes()), 10);
}

TEST_F(BufferStoreTest, serDeserIngressPriorityGroup) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  auto ingressPriorityGroupId = createIngressPriorityGroup(profileId);
  verifyAdapterKeySerDeser<SaiIngressPriorityGroupTraits>(
      {ingressPriorityGroupId});
}

TEST_F(BufferStoreTest, toStrIngressPriorityGroup) {
  auto poolId = createBufferPool();
  auto profileId = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId);
  std::ignore = createIngressPriorityGroup(profileId);
  verifyToStr<SaiIngressPriorityGroupTraits>();
}
