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
#include <memory>

#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>

#include "fboss/lib/IOStatsRecorder.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/module/I2cLogBuffer.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"

namespace facebook {
namespace fboss {

/*
 * This is the Wedge Platform Specific Class
 * and contains all the Wedge QSFP Specific Functions
 */
class WedgeQsfp : public TransceiverImpl {
 public:
  WedgeQsfp(
      int module,
      TransceiverI2CApi* i2c,
      TransceiverManager* const tcvrManager,
      std::unique_ptr<I2cLogBuffer> logBuffer);

  ~WedgeQsfp() override;

  /* This function is used to read the SFP EEprom */
  int readTransceiver(
      const TransceiverAccessParameter& param,
      uint8_t* fieldValue,
      const int field) override;

  /* write to the eeprom (usually to change the page setting) */
  int writeTransceiver(
      const TransceiverAccessParameter& param,
      const uint8_t* fieldValue,
      uint64_t delay,
      const int field) override;

  /* Returns the name for the port */
  folly::StringPiece getName() override;

  int getNum() const override;

  std::optional<TransceiverStats> getTransceiverStats() override;

  folly::EventBase* getI2cEventBase() override;

  TransceiverManagementInterface getTransceiverManagementInterface();
  folly::Future<TransceiverManagementInterface>
  futureGetTransceiverManagementInterface();

  std::array<uint8_t, 16> getModulePartNo();

  std::array<uint8_t, 2> getFirmwareVer();

  // Note that the module_ starts at 0, but the I2C bus module
  // assumes that QSFP module numbers extend from 1 to N.

  /* Detects if a SFP is present on the particular port
   */
  bool detectTransceiver() override {
    return threadSafeI2CBus_->isPresent(module_ + 1);
  }

  /* Unset the reset status of Qsfp
   */
  void ensureOutOfReset() override {
    threadSafeI2CBus_->ensureOutOfReset(module_ + 1);
  }

  /* Functions relevant to I2C Profiling
   */
  void i2cTimeProfilingStart() const override {
    threadSafeI2CBus_->i2cTimeProfilingStart(module_ + 1);
  }

  void i2cTimeProfilingEnd() const override {
    threadSafeI2CBus_->i2cTimeProfilingEnd(module_ + 1);
  }

  std::pair<uint64_t, uint64_t> getI2cTimeProfileMsec() const override {
    return threadSafeI2CBus_->getI2cTimeProfileMsec(module_ + 1);
  }

  void triggerQsfpHardReset() override {
    tcvrManager_->getQsfpPlatformApi()->triggerQsfpHardReset(module_ + 1);
  }

  void updateTransceiverState(TransceiverStateMachineEvent event) override {
    tcvrManager_->updateStateBlocking(TransceiverID(module_), event);
  }

  // Dump i2c log to file. Return number of header/log entries.
  std::pair<size_t, size_t> dumpTransceiverI2cLog();

  // Get the capacity of the i2c buffer.
  size_t getI2cLogBufferCapacity();

  void setTcvrInfoInLog(
      const TransceiverManagementInterface& mgmtIf,
      const std::set<std::string>& portNames,
      const std::optional<FirmwareStatus>& status,
      const std::optional<Vendor>& vendor);

 private:
  int module_;
  std::string moduleName_;
  TransceiverI2CApi* threadSafeI2CBus_;
  TransceiverManager* const tcvrManager_;
  IOStatsRecorder ioStatsRecorder_;
  std::unique_ptr<I2cLogBuffer> logBuffer_;
};

} // namespace fboss
} // namespace facebook
