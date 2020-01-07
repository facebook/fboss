/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <gtest/gtest.h>

#include "FakePhysicalMemory.h"

namespace {
const uint64_t kFakePhysicalAddr = 0xfb000000;
const uint32_t kFakeSize = 4096;
} // namespace

namespace facebook::fboss {
TEST(PhysicalMemoryTest, lock) {
  // Check parameters
  EXPECT_THROW(
      FakePhysicalMemory(kFakePhysicalAddr - 1, kFakeSize), std::system_error);
  EXPECT_THROW(
      FakePhysicalMemory(kFakePhysicalAddr, kFakeSize - 1), std::system_error);

  {
    FakePhysicalMemory p1{kFakePhysicalAddr, kFakeSize};
    // a new object w/ the same addr will throw. That depends on flock()
    EXPECT_THROW(
        FakePhysicalMemory(kFakePhysicalAddr, kFakeSize), std::system_error);
    // if it is OK to fail lock, that's fine
    FakePhysicalMemory p2{kFakePhysicalAddr, kFakeSize, false};
  }

  {
    // The previous lock shall be removed when the object was freed
    FakePhysicalMemory p1{kFakePhysicalAddr, kFakeSize};
  }
}

TEST(PhysicalMemoryTest, io) {
  auto testIO = [](auto& pm, int step) {
    constexpr auto OFFSET = 100;
    pm.mmap();
    for (auto i = 0; i < 4; i++) {
      pm.write(OFFSET + i * step, OFFSET + i);
    }
    for (auto i = 0; i < 4; i++) {
      EXPECT_EQ(pm.read(OFFSET + i * step), OFFSET + i);
    }
  };

  {
    FakePhysicalMemory8 p8{kFakePhysicalAddr, kFakeSize};
    testIO(p8, sizeof(uint8_t));
  }

  {
    FakePhysicalMemory16 p16{kFakePhysicalAddr, kFakeSize};
    testIO(p16, sizeof(uint16_t));
  }

  {
    FakePhysicalMemory32 p32{kFakePhysicalAddr, kFakeSize};
    testIO(p32, sizeof(uint32_t));
  }
}

} // namespace facebook::fboss
