/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/lib/PhysicalMemory.h"

#include <vector>

namespace facebook::fboss {
class FakePhysicalMemory : public PhysicalMemory {
 public:
  using PhysicalMemory::PhysicalMemory;
  ~FakePhysicalMemory() {
    munmap();
  }
  void mmap() {
    data_.resize(getSize());
    setVirtualAddress(data_.data());
  }

 private:
  void munmap() noexcept {
    data_.clear();
    setVirtualAddress(nullptr);
  }
  std::vector<uint8_t> data_;
};

using FakePhysicalMemory8 = PhysicalMemory8<FakePhysicalMemory>;
using FakePhysicalMemory16 = PhysicalMemory16<FakePhysicalMemory>;
using FakePhysicalMemory32 = PhysicalMemory32<FakePhysicalMemory>;

} // namespace facebook::fboss
