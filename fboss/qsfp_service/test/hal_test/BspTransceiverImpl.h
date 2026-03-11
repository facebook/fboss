// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <string>

#include "fboss/lib/bsp/BspTransceiverContainer.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"

namespace facebook::fboss {

/*
 * BspTransceiverImpl implements TransceiverImpl by delegating to
 * BspTransceiverContainer for direct I2C access to transceiver hardware
 * via BSP classes. This bypasses the WedgeManager + state machine stack,
 * enabling lightweight HAL-level testing on real hardware.
 */
class BspTransceiverImpl : public TransceiverImpl {
 public:
  BspTransceiverImpl(
      int module,
      std::string name,
      BspTransceiverMapping bspMapping);

  int readTransceiver(
      const TransceiverAccessParameter& param,
      uint8_t* fieldValue,
      const int field) override;

  int writeTransceiver(
      const TransceiverAccessParameter& param,
      const uint8_t* fieldValue,
      uint64_t delay,
      const int field) override;

  bool detectTransceiver() override;

  void triggerQsfpHardReset() override;

  folly::StringPiece getName() override;

  int getNum() const override;

  void ensureOutOfReset() override;

  void i2cTimeProfilingStart() const override;
  void i2cTimeProfilingEnd() const override;
  std::pair<uint64_t, uint64_t> getI2cTimeProfileMsec() const override;

 private:
  int module_;
  std::string moduleName_;
  std::unique_ptr<BspTransceiverContainer> tcvrContainer_;
};

} // namespace facebook::fboss
