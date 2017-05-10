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

#include <string>

namespace facebook {
namespace fboss {

class BufferStatsLogger {
 public:
  enum class Direction { Ingress, Egress };
  virtual ~BufferStatsLogger() {}
  virtual void logDeviceBufferStat(uint64_t bytesUsed, uint64_t bytesMax) = 0;
  virtual void logPortBufferStat(
      const std::string& portName,
      Direction dir,
      uint64_t bytesUsed) = 0;
  static std::string dirStr(Direction dir) {
    switch (dir) {
      case Direction::Ingress:
        return "Ingress";
      case Direction::Egress:
        return "Egress";
    }
    return "Unknown";
  }
};

class GlogBufferStatsLogger : public BufferStatsLogger {
 public:
  void logDeviceBufferStat(uint64_t bytesUsed, uint64_t bytesMax) override;
  void logPortBufferStat(
      const std::string& portName,
      Direction dir, uint64_t bytesUsed) override;
};
}
}
