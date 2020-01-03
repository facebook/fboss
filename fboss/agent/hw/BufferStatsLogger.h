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

#include <folly/String.h>
#include <string>
#include <vector>

namespace facebook::fboss {

class BufferStatsLogger {
 public:
  enum class Direction { Ingress, Egress };
  using XPEs = std::vector<unsigned int>;

  virtual ~BufferStatsLogger() {}
  virtual void logDeviceBufferStat(uint64_t bytesUsed, uint64_t bytesMax) = 0;
  virtual void logPortBufferStat(
      const std::string& portName,
      Direction dir,
      unsigned int cosQ,
      uint64_t bytesUsed,
      uint64_t pktsDropped,
      const XPEs& xpes) = 0;
  static std::string dirStr(Direction dir) {
    switch (dir) {
      case Direction::Ingress:
        return "Ingress";
      case Direction::Egress:
        return "Egress";
    }
    return "Unknown";
  }
  std::string xpeStr(const XPEs& xpes) const {
    return folly::join(",", xpes);
  }
};

class GlogBufferStatsLogger : public BufferStatsLogger {
 public:
  void logDeviceBufferStat(uint64_t bytesUsed, uint64_t bytesMax) override;
  void logPortBufferStat(
      const std::string& portName,
      Direction dir,
      unsigned int cosQ,
      uint64_t bytesUsed,
      uint64_t pktsDropped,
      const XPEs& xpes) override;
};

} // namespace facebook::fboss
