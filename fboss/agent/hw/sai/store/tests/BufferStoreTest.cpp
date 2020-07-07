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
  SaiBufferPoolTraits::CreateAttributes createAttrs() const {
    SaiBufferPoolTraits::Attributes::PoolType type{SAI_BUFFER_POOL_TYPE_EGRESS};
    SaiBufferPoolTraits::Attributes::PoolSize size{42};
    SaiBufferPoolTraits::Attributes::ThresholdMode mode{
        SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC};
    return {type, size, mode};
  }
  BufferPoolSaiId createBufferPool() const {
    auto& bufferApi = saiApiTable->bufferApi();
    return bufferApi.create<SaiBufferPoolTraits>(createAttrs(), 0);
  }
};

TEST_F(BufferStoreTest, loadBufferPool) {
  auto poolId = createBufferPool();
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiBufferPoolTraits>();
  auto got = store.get(SAI_BUFFER_POOL_TYPE_EGRESS);
  EXPECT_EQ(got->adapterKey(), poolId);
  EXPECT_EQ(
      std::get<SaiBufferPoolTraits::Attributes::ThresholdMode>(
          got->attributes()),
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
}

TEST_F(BufferStoreTest, loadBufferPoolFromJson) {
  auto poolId = createBufferPool();
  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  SaiStore s2(0);
  s2.reload(&json);
  auto& store = s2.get<SaiBufferPoolTraits>();
  auto got = store.get(SAI_BUFFER_POOL_TYPE_EGRESS);
  EXPECT_EQ(got->adapterKey(), poolId);
  EXPECT_EQ(
      std::get<SaiBufferPoolTraits::Attributes::ThresholdMode>(
          got->attributes()),
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC);
}
/*
 * Note, the default bridge id is a pain to get at, so we will skip
 * the bridgeLoadCtor test. That will largely be covered by testing
 * BridgeStore.reload() anyway.
 */
TEST_F(BufferStoreTest, bufferPoolLoadCtor) {
  auto poolId = createBufferPool();
  SaiObject<SaiBufferPoolTraits> obj(poolId);
  EXPECT_EQ(obj.adapterKey(), poolId);
  EXPECT_EQ(GET_ATTR(BufferPool, PoolSize, obj.attributes()), 42);
}

TEST_F(BufferStoreTest, bufferPoolCreateCtor) {
  SaiObject<SaiBufferPoolTraits> obj({24}, createAttrs(), 0);
  EXPECT_EQ(GET_ATTR(BufferPool, PoolSize, obj.attributes()), 42);
}

TEST_F(BufferStoreTest, serDeserBufferPool) {
  auto poolId = createBufferPool();
  verifyAdapterKeySerDeser<SaiBufferPoolTraits>({poolId});
}
