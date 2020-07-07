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
    SaiBufferPoolTraits::Attributes::PoolType type{_type};
    SaiBufferPoolTraits::Attributes::PoolSize size{_size};
    SaiBufferPoolTraits::Attributes::ThresholdMode mode{_mode};
    SaiBufferPoolTraits::CreateAttributes c{type, size, mode};
    return bufferApi->create<SaiBufferPoolTraits>(c, 0);
  }
  void checkBufferPool(BufferPoolSaiId id) const {
    SaiBufferPoolTraits::Attributes::PoolType type;
    auto gotType = bufferApi->getAttribute(id, type);
    EXPECT_EQ(fs->bufferPoolManager.get(id).poolType, gotType);
    SaiBufferPoolTraits::Attributes::PoolSize size;
    auto gotSize = bufferApi->getAttribute(id, size);
    EXPECT_EQ(fs->bufferPoolManager.get(id).size, gotSize);
    SaiBufferPoolTraits::Attributes::ThresholdMode mode;
    auto gotMode = bufferApi->getAttribute(id, mode);
    EXPECT_EQ(fs->bufferPoolManager.get(id).threshMode, gotMode);
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
      SAI_BUFFER_POOL_TYPE_INGRESS, 42, SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferPoolTraits::Attributes::PoolType{}),
      SAI_BUFFER_POOL_TYPE_INGRESS);
  EXPECT_EQ(
      bufferApi->getAttribute(id, SaiBufferPoolTraits::Attributes::PoolSize{}),
      42);
  EXPECT_EQ(
      bufferApi->getAttribute(
          id, SaiBufferPoolTraits::Attributes::ThresholdMode{}),
      SAI_BUFFER_POOL_THRESHOLD_MODE_STATIC);
}
