/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/EncapIndexAllocator.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/switch_asics/MockAsic.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class EncapIndexAllocatorTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA(cfg::SwitchType::VOQ);
    handle = createTestHandle(&config);
  }
  SwSwitch* getSw() const {
    return handle->getSw();
  }
  const HwAsic& getAsic() const {
    return *getSw()->getPlatform()->getAsic();
  }

  int64_t allocationStart() const {
    return *getAsic().getReservedEncapIndexRange().minimum() +
        EncapIndexAllocator::kEncapIdxReservedForLoopbacks;
  }

  std::unique_ptr<HwTestHandle> handle;
  EncapIndexAllocator allocator;
};

TEST_F(EncapIndexAllocatorTest, unsupportedAsic) {
  auto asic =
      std::make_unique<TomahawkAsic>(cfg::SwitchType::NPU, std::nullopt);
  EXPECT_THROW(allocator.getNextAvailableEncapIdx(nullptr, *asic), FbossError);
}

TEST_F(EncapIndexAllocatorTest, firstAllocation) {
  EXPECT_EQ(
      allocator.getNextAvailableEncapIdx(getSw()->getState(), getAsic()),
      allocationStart());
}
