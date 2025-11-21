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

#include <gtest/gtest.h>

using namespace facebook::fboss;

class BufferApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    bufferApi = std::make_unique<BufferApi>();
  }
  BufferPoolSaiId createBufferPool(
      sai_buffer_pool_type_t _type,
      sai_uint64_t _size,
      sai_buffer_pool_threshold_mode_t _mode) {
    SaiBufferPoolTraits::Attributes::Type type{_type};
    SaiBufferPoolTraits::Attributes::Size size{_size};
    SaiBufferPoolTraits::Attributes::ThresholdMode mode{_mode};
    std::optional<SaiBufferPoolTraits::Attributes::XoffSize> xoffSize;
    SaiBufferPoolTraits::CreateAttributes c{type, size, mode, xoffSize};
    return bufferApi->create<SaiBufferPoolTraits>(c, 0);
  }
  void checkBufferPool(BufferPoolSaiId id) const {
    SaiBufferPoolTraits::Attributes::Type type;
    auto gotType = bufferApi->getAttribute(id, type);
    EXPECT_EQ(fs->bufferPoolManager.get(id).poolType, gotType);
    SaiBufferPoolTraits::Attributes::Size size;
    auto gotSize = bufferApi->getAttribute(id, size);
    EXPECT_EQ(fs->bufferPoolManager.get(id).size, gotSize);
    SaiBufferPoolTraits::Attributes::ThresholdMode mode;
    auto gotMode = bufferApi->getAttribute(id, mode);
    EXPECT_EQ(fs->bufferPoolManager.get(id).threshMode, gotMode);
  }

  template <typename BufferProfileTraits>
  BufferProfileSaiId createBufferProfile(
      BufferPoolSaiId _pool,
      bool ingressBufferProfile = false) {
    SaiBufferProfileAttributes::PoolId pool{_pool};
    SaiBufferProfileAttributes::ReservedBytes reservedBytes{42};
    SaiBufferProfileAttributes::ThresholdMode mode;
    SaiBufferProfileAttributes::XoffTh xoffTh{0};
    SaiBufferProfileAttributes::XonTh xonTh{0};
    SaiBufferProfileAttributes::XonOffsetTh xonOffsetTh{0};
    if (ingressBufferProfile) {
      xoffTh = SaiBufferProfileAttributes::XoffTh{293624};
      xonOffsetTh = SaiBufferProfileAttributes::XonOffsetTh{4826};
    }
    if constexpr (std::is_same_v<
                      BufferProfileTraits,
                      SaiDynamicBufferProfileTraits>) {
      mode = SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC;
      SaiDynamicBufferProfileTraits::Attributes::SharedDynamicThreshold
          dynamicThresh{24};
      SaiDynamicBufferProfileTraits::CreateAttributes c{
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
      return bufferApi->create<SaiDynamicBufferProfileTraits>(c, 0);
    } else {
      mode = SAI_BUFFER_PROFILE_THRESHOLD_MODE_STATIC;
      SaiStaticBufferProfileTraits::Attributes::SharedStaticThreshold
          staticThresh{34};
      SaiStaticBufferProfileTraits::CreateAttributes c{
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
      return bufferApi->create<SaiStaticBufferProfileTraits>(c, 0);
    }
  }

  template <typename BufferProfileTraits>
  void checkBufferProfile(BufferProfileSaiId id) const {
    SaiBufferProfileAttributes::PoolId pool{};
    auto gotPool = bufferApi->getAttribute(id, pool);
    EXPECT_EQ(fs->bufferProfileManager.get(id).poolId, gotPool);
    SaiBufferProfileAttributes::ReservedBytes reservedBytes{};
    auto gotReservedBytes = bufferApi->getAttribute(id, reservedBytes);
    EXPECT_EQ(fs->bufferProfileManager.get(id).reservedBytes, gotReservedBytes);
    SaiBufferProfileAttributes::ThresholdMode mode{};
    auto gotThresholdMode = bufferApi->getAttribute(id, mode);
    EXPECT_EQ(fs->bufferProfileManager.get(id).threshMode, gotThresholdMode);

    if constexpr (std::is_same_v<
                      BufferProfileTraits,
                      SaiDynamicBufferProfileTraits>) {
      SaiDynamicBufferProfileTraits::Attributes::SharedDynamicThreshold
          dynamicThresh{};
      auto gotDynamic = bufferApi->getAttribute(id, dynamicThresh);
      EXPECT_EQ(fs->bufferProfileManager.get(id).dynamicThreshold, gotDynamic);
    } else {
      SaiStaticBufferProfileTraits::Attributes::SharedStaticThreshold
          staticThresh{};
      auto gotStatic = bufferApi->getAttribute(id, staticThresh);
      EXPECT_EQ(fs->bufferProfileManager.get(id).staticThreshold, gotStatic);
    }

    SaiBufferProfileAttributes::XoffTh xoffTh{};
    auto gotXoffTh = bufferApi->getAttribute(id, xoffTh);
    auto expectedXoffTh = fs->bufferProfileManager.get(id).xoffTh.has_value()
        ? fs->bufferProfileManager.get(id).xoffTh.value()
        : 0;
    EXPECT_EQ(expectedXoffTh, gotXoffTh);
    SaiBufferProfileAttributes::XonTh xonTh{};
    auto gotXonTh = bufferApi->getAttribute(id, xonTh);
    auto expectedXonTh = fs->bufferProfileManager.get(id).xonTh.has_value()
        ? fs->bufferProfileManager.get(id).xonTh.value()
        : 0;
    EXPECT_EQ(expectedXonTh, gotXonTh);
    SaiBufferProfileAttributes::XonOffsetTh xonOffsetTh{};
    auto gotXonOffsetTh = bufferApi->getAttribute(id, xonOffsetTh);
    auto expectedXonOffsetTh =
        fs->bufferProfileManager.get(id).xonOffsetTh.has_value()
        ? fs->bufferProfileManager.get(id).xonOffsetTh.value()
        : 0;
    EXPECT_EQ(expectedXonOffsetTh, gotXonOffsetTh);
  }

  IngressPriorityGroupSaiId createIngressPriorityGroup() {
    SaiIngressPriorityGroupTraits::Attributes::Port port{1};
    SaiIngressPriorityGroupTraits::Attributes::Index index{10};
    std::optional<SaiIngressPriorityGroupTraits::Attributes::BufferProfile>
        bufferProfile;
    std::optional<SaiIngressPriorityGroupTraits::Attributes::LosslessEnable>
        losslessEnable;

    SaiIngressPriorityGroupTraits::CreateAttributes c{
        port, index, bufferProfile, losslessEnable};
    return bufferApi->create<SaiIngressPriorityGroupTraits>(c, 0);
  }
  void checkIngressPriorityGroup(IngressPriorityGroupSaiId id) const {
    SaiIngressPriorityGroupTraits::Attributes::Port port{};
    auto gotPort = bufferApi->getAttribute(id, port);
    EXPECT_EQ(fs->ingressPriorityGroupManager.get(id).port, gotPort);

    SaiIngressPriorityGroupTraits::Attributes::Index index{};
    auto gotIndex = bufferApi->getAttribute(id, index);
    EXPECT_EQ(fs->ingressPriorityGroupManager.get(id).index, gotIndex);

    SaiIngressPriorityGroupTraits::Attributes::BufferProfile bufferProfile{};
    auto gotBufferProfile = bufferApi->getAttribute(id, bufferProfile);
    auto expectedBufferProfile =
        fs->ingressPriorityGroupManager.get(id).bufferProfile.has_value()
        ? fs->ingressPriorityGroupManager.get(id).bufferProfile.value()
        : 0;
    EXPECT_EQ(expectedBufferProfile, gotBufferProfile);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<BufferApi> bufferApi;
};

TEST_F(BufferApiTest, createBufferPool) {
  auto saiBufferId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_EGRESS,
      1024,
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
  checkBufferPool(saiBufferId);
}

TEST_F(BufferApiTest, getBufferPoolAttributes) {
  auto id = createBufferPool(
      SAI_BUFFER_POOL_TYPE_INGRESS, 42, SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferPoolTraits::Attributes::Type{}),
      SAI_BUFFER_POOL_TYPE_INGRESS);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferPoolTraits::Attributes::Size{}), 42);
  EXPECT_EQ(
      bufferApi->getAttribute(
          id, SaiBufferPoolTraits::Attributes::ThresholdMode{}),
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
}

TEST_F(BufferApiTest, createBufferProfileDynamic) {
  auto saiBufferId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_EGRESS,
      1024,
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
  auto profileId =
      createBufferProfile<SaiDynamicBufferProfileTraits>(saiBufferId);
  checkBufferProfile<SaiDynamicBufferProfileTraits>(profileId);
}

TEST_F(BufferApiTest, createBufferProfileStatis) {
  auto saiBufferId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_EGRESS, 1024, SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC);
  auto profileId =
      createBufferProfile<SaiStaticBufferProfileTraits>(saiBufferId);
  checkBufferProfile<SaiStaticBufferProfileTraits>(profileId);
}

TEST_F(BufferApiTest, createBufferProfileIngressDynamic) {
  auto saiBufferId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_INGRESS,
      1024,
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
  auto profileId =
      createBufferProfile<SaiDynamicBufferProfileTraits>(saiBufferId, true);
  checkBufferProfile<SaiDynamicBufferProfileTraits>(profileId);
}

TEST_F(BufferApiTest, createBufferProfileIngressStatic) {
  auto saiBufferId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_INGRESS,
      1024,
      SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC);
  auto profileId =
      createBufferProfile<SaiStaticBufferProfileTraits>(saiBufferId, true);
  checkBufferProfile<SaiStaticBufferProfileTraits>(profileId);
}

TEST_F(BufferApiTest, getBufferProfilelAttributesDynamic) {
  auto poolId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_INGRESS, 42, SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
  auto id = createBufferProfile<SaiDynamicBufferProfileTraits>(poolId, true);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::PoolId{}),
      poolId);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::ReservedBytes{}),
      42);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::ThresholdMode{}),
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC);
  EXPECT_EQ(
      bufferApi->getAttribute(
          id,
          SaiDynamicBufferProfileTraits::Attributes::SharedDynamicThreshold{}),
      24);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::XoffTh{}),
      293624);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::XonTh{}), 0);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::XonOffsetTh{}),
      4826);
}

TEST_F(BufferApiTest, getBufferProfilelAttributesStatic) {
  auto poolId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_INGRESS, 42, SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC);
  auto id = createBufferProfile<SaiStaticBufferProfileTraits>(poolId, true);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::PoolId{}),
      poolId);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::ReservedBytes{}),
      42);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::ThresholdMode{}),
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_STATIC);
  EXPECT_EQ(
      bufferApi->getAttribute(
          id,
          SaiStaticBufferProfileTraits::Attributes::SharedStaticThreshold{}),
      34);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::XoffTh{}),
      293624);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::XonTh{}), 0);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferProfileAttributes::XonOffsetTh{}),
      4826);
}

TEST_F(BufferApiTest, setBufferProfileAttributesDynamic) {
  auto saiBufferId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_EGRESS,
      1024,
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
  auto profileId =
      createBufferProfile<SaiDynamicBufferProfileTraits>(saiBufferId);
  bufferApi->setAttribute(
      profileId, SaiBufferProfileAttributes::ReservedBytes{24});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId, SaiBufferProfileAttributes::ReservedBytes{}),
      24);

  bufferApi->setAttribute(
      profileId,
      SaiBufferProfileAttributes::ThresholdMode{
          SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId, SaiBufferProfileAttributes::ThresholdMode{}),
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC);

  bufferApi->setAttribute(
      profileId,
      SaiDynamicBufferProfileTraits::Attributes::SharedDynamicThreshold{42});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId,
          SaiDynamicBufferProfileTraits::Attributes::SharedDynamicThreshold{}),
      42);
}

TEST_F(BufferApiTest, setBufferProfileAttributesStatic) {
  auto saiBufferId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_EGRESS, 1024, SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC);
  auto profileId =
      createBufferProfile<SaiStaticBufferProfileTraits>(saiBufferId);
  bufferApi->setAttribute(
      profileId, SaiBufferProfileAttributes::ReservedBytes{24});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId, SaiBufferProfileAttributes::ReservedBytes{}),
      24);

  bufferApi->setAttribute(
      profileId,
      SaiBufferProfileAttributes::ThresholdMode{
          SAI_BUFFER_PROFILE_THRESHOLD_MODE_STATIC});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId, SaiBufferProfileAttributes::ThresholdMode{}),
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_STATIC);

  bufferApi->setAttribute(
      profileId,
      SaiStaticBufferProfileTraits::Attributes::SharedStaticThreshold{42});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId,
          SaiStaticBufferProfileTraits::Attributes::SharedStaticThreshold{}),
      42);
}

TEST_F(BufferApiTest, setBufferProfileAttributesIngressDynamic) {
  auto saiBufferId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_INGRESS,
      1024,
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
  auto profileId =
      createBufferProfile<SaiDynamicBufferProfileTraits>(saiBufferId, true);
  bufferApi->setAttribute(
      profileId, SaiBufferProfileAttributes::ReservedBytes{24});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId, SaiBufferProfileAttributes::ReservedBytes{}),
      24);

  bufferApi->setAttribute(
      profileId,
      SaiBufferProfileAttributes::ThresholdMode{
          SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId, SaiBufferProfileAttributes::ThresholdMode{}),
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_DYNAMIC);

  bufferApi->setAttribute(
      profileId,
      SaiDynamicBufferProfileTraits::Attributes::SharedDynamicThreshold{42});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId,
          SaiDynamicBufferProfileTraits::Attributes::SharedDynamicThreshold{}),
      42);
  bufferApi->setAttribute(
      profileId, SaiBufferProfileAttributes::XoffTh{293624});
  EXPECT_EQ(
      bufferApi->getAttribute(profileId, SaiBufferProfileAttributes::XoffTh{}),
      293624);
  bufferApi->setAttribute(profileId, SaiBufferProfileAttributes::XonTh{0});
  EXPECT_EQ(
      bufferApi->getAttribute(profileId, SaiBufferProfileAttributes::XonTh{}),
      0);
  bufferApi->setAttribute(
      profileId, SaiBufferProfileAttributes::XonOffsetTh{4826});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId, SaiBufferProfileAttributes::XonOffsetTh{}),
      4826);
}

TEST_F(BufferApiTest, setBufferProfileAttributesIngressStatic) {
  auto saiBufferId = createBufferPool(
      SAI_BUFFER_POOL_TYPE_INGRESS,
      1024,
      SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC);
  auto profileId =
      createBufferProfile<SaiStaticBufferProfileTraits>(saiBufferId, true);
  bufferApi->setAttribute(
      profileId, SaiBufferProfileAttributes::ReservedBytes{24});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId, SaiBufferProfileAttributes::ReservedBytes{}),
      24);

  bufferApi->setAttribute(
      profileId,
      SaiBufferProfileAttributes::ThresholdMode{
          SAI_BUFFER_PROFILE_THRESHOLD_MODE_STATIC});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId, SaiBufferProfileAttributes::ThresholdMode{}),
      SAI_BUFFER_PROFILE_THRESHOLD_MODE_STATIC);

  bufferApi->setAttribute(
      profileId,
      SaiStaticBufferProfileTraits::Attributes::SharedStaticThreshold{42});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId,
          SaiStaticBufferProfileTraits::Attributes::SharedStaticThreshold{}),
      42);
  bufferApi->setAttribute(
      profileId, SaiBufferProfileAttributes::XoffTh{293624});
  EXPECT_EQ(
      bufferApi->getAttribute(profileId, SaiBufferProfileAttributes::XoffTh{}),
      293624);
  bufferApi->setAttribute(profileId, SaiBufferProfileAttributes::XonTh{0});
  EXPECT_EQ(
      bufferApi->getAttribute(profileId, SaiBufferProfileAttributes::XonTh{}),
      0);
  bufferApi->setAttribute(
      profileId, SaiBufferProfileAttributes::XonOffsetTh{4826});
  EXPECT_EQ(
      bufferApi->getAttribute(
          profileId, SaiBufferProfileAttributes::XonOffsetTh{}),
      4826);
}

TEST_F(BufferApiTest, createIngressPriorityGroup) {
  auto ingressPriorityGroupId = createIngressPriorityGroup();
  checkIngressPriorityGroup(ingressPriorityGroupId);
}
TEST_F(BufferApiTest, getIngressPriorityGroupAttritbutes) {
  auto id = createIngressPriorityGroup();
  EXPECT_EQ(
      bufferApi->getAttribute(
          id, SaiIngressPriorityGroupTraits::Attributes::Port{}),
      1);
  EXPECT_EQ(
      bufferApi->getAttribute(
          id, SaiIngressPriorityGroupTraits::Attributes::Index{}),
      10);
}
TEST_F(BufferApiTest, setIngressPriorityGroupAttritbutes) {
  auto id = createIngressPriorityGroup();
  bufferApi->setAttribute(
      id, SaiIngressPriorityGroupTraits::Attributes::Port{2});
  EXPECT_EQ(
      bufferApi->getAttribute(
          id, SaiIngressPriorityGroupTraits::Attributes::Port{}),
      2);
  bufferApi->setAttribute(
      id, SaiIngressPriorityGroupTraits::Attributes::Index{20});
  EXPECT_EQ(
      bufferApi->getAttribute(
          id, SaiIngressPriorityGroupTraits::Attributes::Index{}),
      20);
  std::optional<SaiIngressPriorityGroupTraits::Attributes::BufferProfile>
      bufferProfile{100};
  bufferApi->setAttribute(id, bufferProfile);
  EXPECT_EQ(
      bufferApi->getAttribute(
          id, SaiIngressPriorityGroupTraits::Attributes::BufferProfile{}),
      100);
}

TEST_F(BufferApiTest, StdNullOptDefault) {
  static_assert(
      SaiDynamicBufferProfileTraits::Attributes::SharedDynamicThreshold::
          hasOptionalDefaultGetter,
      "SharedStaticThreshold hasOptionalDefaultGetter");
  auto defaultVal = SaiDynamicBufferProfileTraits::Attributes::
      SharedDynamicThreshold::defaultValue();
  EXPECT_EQ(defaultVal, std::nullopt);
}
