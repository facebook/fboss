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
#include <optional>

#include <folly/String.h>
#include <folly/io/async/EventBase.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/types.h"
#include "fboss/lib/usb/TransceiverAccessParameter.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/Transceiver.h"

namespace facebook {
namespace fboss {

constexpr uint64_t POST_I2C_WRITE_DELAY_US = 20000;
constexpr uint64_t POST_I2C_WRITE_DELAY_CDB_US = 5000;
constexpr uint64_t POST_I2C_WRITE_NO_DELAY_US = 0;

/*
 * This is class is the SFP implementation class
 */
class TransceiverImpl {
 public:
  TransceiverImpl() {}
  virtual ~TransceiverImpl() {}

  /*
   * Get the raw data from the Transceiver via I2C
   */
  virtual int readTransceiver(
      const TransceiverAccessParameter& param,
      uint8_t* fieldValue,
      const int field) = 0;

  /*
   * Write to a tranceiver with a specific delay post write.
   * Implementer should add the specified delay (e.g. usleep) after
   * the write.
   * If delay is not specified then the default delay should be
   * used.
   */
  virtual int writeTransceiver(
      const TransceiverAccessParameter& param,
      const uint8_t* fieldValue,
      uint64_t delay,
      const int field) = 0;

  /*
   * This function will check if the transceiver is present or not
   */
  virtual bool detectTransceiver() = 0;

  /* This function does a hard reset of the QSFP and this will be
   * called when port flap is seen on the port remains down
   */
  virtual void triggerQsfpHardReset() = 0;

  /*
   * Returns the name of the port
   */
  virtual folly::StringPiece getName() = 0;

  /*
   * Returns the index of the transceiver (0 based)
   */
  virtual int getNum() const = 0;

  /*
   * In an implementation where newly inserted transceivers needs to be cleared
   * out of reset. This is the function to do it.
   */
  virtual void ensureOutOfReset() {}

  /* Functions relevant to I2C Profiling
   */
  virtual void i2cTimeProfilingStart() const {}
  virtual void i2cTimeProfilingEnd() const {}
  virtual std::pair<uint64_t, uint64_t> getI2cTimeProfileMsec() const {
    return std::make_pair(0, 0);
  }

  virtual std::optional<TransceiverStats> getTransceiverStats() {
    return std::optional<TransceiverStats>();
  }

  /*
   * Function that returns the eventbase that suppose to execute the I2C txn
   * associated with the module. At this moment, only Minipack and Yamp which
   * are the models using FPGA has the ability of doing I2C txn parallelly.
   * Other models that has single I2C bus will return nullptr by default which
   * means no relevant eventbase.
   */
  virtual folly::EventBase* getI2cEventBase() {
    return nullptr;
  }

  virtual void updateTransceiverState(TransceiverStateMachineEvent /*event*/) {}

 private:
  // Forbidden copy contructor and assignment operator
  TransceiverImpl(TransceiverImpl const&) = delete;
  TransceiverImpl& operator=(TransceiverImpl const&) = delete;

  enum : unsigned int { kMaxChannels = 4 };
};

} // namespace fboss
} // namespace facebook
