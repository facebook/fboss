// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <folly/logging/xlog.h>
#include <memory>
#include <unordered_map>
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/BspTransceiverAccess.h"
#include "fboss/lib/bsp/BspTransceiverIO.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook {
namespace fboss {

class BspTransceiverContainer {
 public:
  explicit BspTransceiverContainer(BspTransceiverMapping& tcvrMapping);

  bool isTcvrPresent() const;
  void initTransceiver() {}
  void releaseTransceiverReset() const;
  void holdTransceiverReset() const;
  void initTcvr() const;
  void tcvrRead(const TransceiverAccessParameter& param, uint8_t* buf) const;
  void tcvrWrite(const TransceiverAccessParameter& param, const uint8_t* buf)
      const;
  const I2cControllerStats getI2cControllerStats() const;
  void i2cTimeProfilingStart() const;
  void i2cTimeProfilingEnd() const;
  std::pair<uint64_t, uint64_t> getI2cTimeProfileMsec() const;

 private:
  BspTransceiverMapping tcvrMapping_;
  int tcvrID_;
  std::unique_ptr<BspTransceiverIO> tcvrIO_;
  std::unique_ptr<BspTransceiverAccess> tcvrAccess_;
};

} // namespace fboss
} // namespace facebook
