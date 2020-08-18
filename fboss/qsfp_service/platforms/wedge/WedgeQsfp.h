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

#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>
#include <chrono>
#include <cstdint>
#include <mutex>
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"

namespace facebook { namespace fboss {

class WedgeQsfpStats {
  using time_point = std::chrono::steady_clock::time_point;
  using seconds = std::chrono::seconds;

 public:
  WedgeQsfpStats() {}
  ~WedgeQsfpStats() {}

  void updateReadDownTime() {
    std::lock_guard<std::mutex> g(statsMutex_);
    *stats_.readDownTime_ref() = downTimeLocked(lastSuccessfulRead_);
  }

  void updateWriteDownTime() {
    std::lock_guard<std::mutex> g(statsMutex_);
    *stats_.writeDownTime_ref() = downTimeLocked(lastSuccessfulWrite_);
  }

  void recordReadSuccess() {
    std::lock_guard<std::mutex> g(statsMutex_);
    lastSuccessfulRead_ = std::chrono::steady_clock::now();
  }

  void recordWriteSuccess() {
    std::lock_guard<std::mutex> g(statsMutex_);
    lastSuccessfulWrite_ = std::chrono::steady_clock::now();
  }

  TransceiverStats getStats() {
    std::lock_guard<std::mutex> g(statsMutex_);
    return stats_;
  }

 private:
  double downTimeLocked(const time_point& lastSuccess) {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<seconds>(now - lastSuccess).count();
  }
  TransceiverStats stats_;
  std::mutex statsMutex_;
  time_point lastSuccessfulRead_;
  time_point lastSuccessfulWrite_;
};

/*
 * This is the Wedge Platform Specific Class
 * and contains all the Wedge QSFP Specific Functions
 */
class WedgeQsfp : public TransceiverImpl {
 public:
  WedgeQsfp(int module, TransceiverI2CApi* i2c);

  ~WedgeQsfp() override;

  /* This function is used to read the SFP EEprom */
  int readTransceiver(int dataAddress, int offset,
                      int len, uint8_t* fieldValue) override;

  /* write to the eeprom (usually to change the page setting) */
  int writeTransceiver(int dataAddress, int offset,
                       int len, uint8_t* fieldValue) override;

  /* This function detects if a SFP is present on the particular port */
  bool detectTransceiver() override;

  /* This function unset the reset status of Qsfp */
  void ensureOutOfReset() override;

  /* Returns the name for the port */
  folly::StringPiece getName() override;

  int getNum() const override;

  std::optional<TransceiverStats> getTransceiverStats() override;

  folly::EventBase* getI2cEventBase() override;

  TransceiverManagementInterface getTransceiverManagementInterface();
  folly::Future<TransceiverManagementInterface>
    futureGetTransceiverManagementInterface();

 private:
  int module_;
  std::string moduleName_;
  TransceiverI2CApi* threadSafeI2CBus_;
  WedgeQsfpStats wedgeQsfpstats_;
};

}} // namespace facebook::fboss
