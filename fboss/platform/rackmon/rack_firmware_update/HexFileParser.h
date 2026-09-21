/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace facebook::fboss::platform::rackmon {

/**
 * Represents a contiguous segment of data in a hex file
 */
struct HexSegment {
  uint32_t startAddress;
  std::vector<uint8_t> data;

  size_t size() const {
    return data.size();
  }

  uint32_t endAddress() const {
    return startAddress + data.size();
  }

  bool contains(uint32_t address) const {
    return address >= startAddress && address < endAddress();
  }
};

/**
 * Represents an Intel HEX file with multiple segments
 */
class HexFile {
 public:
  /**
   * Load Intel HEX file from path
   * @param filename Path to .hex file
   * @return HexFile object
   * @throws FileException on parse error
   */
  static HexFile load(const std::string& filename);

  /**
   * Get all segments in the hex file
   */
  const std::vector<HexSegment>& getSegments() const {
    return segments_;
  }

  /**
   * Get total size of all segments
   */
  size_t totalSize() const;

  /**
   * Get entry instruction pointer (EIP) if available
   */
  uint32_t getEIP() const {
    return eip_;
  }

  /**
   * Get code segment (CS) if available
   */
  uint16_t getCS() const {
    return cs_;
  }

  /**
   * Get instruction pointer (IP) if available
   */
  uint16_t getIP() const {
    return ip_;
  }

  /**
   * Check if EIP is set
   */
  bool hasEIP() const {
    return hasEIP_;
  }

  /**
   * Check if CS/IP are set
   */
  bool hasCSIP() const {
    return hasCSIP_;
  }

 private:
  HexFile() = default;

  std::vector<HexSegment> segments_;
  uint32_t eip_{0};
  uint16_t cs_{0};
  uint16_t ip_{0};
  bool hasEIP_{false};
  bool hasCSIP_{false};
};

} // namespace facebook::fboss::platform::rackmon
