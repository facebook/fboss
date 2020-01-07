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

#include <boost/noncopyable.hpp>
#include <glog/logging.h>

#include <folly/File.h>

namespace facebook::fboss {
/**
 * A class represents a physical address region.
 */
class PhysicalMemory : public boost::noncopyable {
 public:
  /**
   * Create an object with the base physical address (phyAddr) and its size.
   * A lock file based on phyAddr will be created and then locked. If the lock
   * file was locked already and mustLock is true, an exception will throw.
   */
  PhysicalMemory(uint64_t phyAddr, uint32_t size, bool mustLock = true);

  virtual ~PhysicalMemory() {
    cleanup();
  }

  /**
   * Map the physical address into the process virtual address space.
   */
  void mmap();

  uint64_t getPhyAddress() const {
    return phyAddr_;
  }

  uint32_t getSize() const {
    return size_;
  }

 protected:
  template <typename ValueT>
  ValueT readImpl(uint32_t offset) const {
    return *atOffset<ValueT>(offset);
  }

  template <typename ValueT>
  void writeImpl(uint32_t offset, ValueT value) {
    *atOffset<ValueT>(offset) = value;
  }

  void setVirtualAddress(void* virtAddr) {
    virtAddr_ = virtAddr;
  }

 private:
  template <typename ValueT>
  auto atOffset(uint32_t offset) const {
    CHECK_LT(offset, size_);
    // offset must be aligned
    CHECK(!(offset & (std::alignment_of<ValueT>::value - 1)));
    return reinterpret_cast<volatile ValueT*>(
        reinterpret_cast<char*>(virtAddr_) + offset);
  }
  void munmap() noexcept;
  void cleanup() noexcept;
  const uint64_t phyAddr_{0};
  const uint32_t size_{0};
  // lock file to prevent other process accessing the same physical memory
  folly::File lockFile_;
  // file to access '/dev/mem'
  folly::File memFile_;
  // virtual address after mmap
  void* virtAddr_{nullptr};
};

template <typename BaseT = PhysicalMemory>
class PhysicalMemory8 : public BaseT {
 public:
  using BaseT::BaseT;
  uint8_t read(uint32_t offset) const {
    return BaseT::template readImpl<uint8_t>(offset);
  }
  void write(uint32_t offset, uint8_t val) {
    return BaseT::template writeImpl(offset, val);
  }
};

template <typename BaseT = PhysicalMemory>
class PhysicalMemory16 : public BaseT {
 public:
  using BaseT::BaseT;
  uint16_t read(uint32_t offset) const {
    return BaseT::template readImpl<uint16_t>(offset);
  }
  void write(uint32_t offset, uint16_t val) {
    return BaseT::template writeImpl(offset, val);
  }
};

template <typename BaseT = PhysicalMemory>
class PhysicalMemory32 : public BaseT {
 public:
  using BaseT::BaseT;
  uint32_t read(uint32_t offset) const {
    return BaseT::template readImpl<uint32_t>(offset);
  }
  void write(uint32_t offset, uint32_t val) {
    return BaseT::template writeImpl(offset, val);
  }
};

} // namespace facebook::fboss
