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

#include <optional>
#include <folly/String.h>
#include <folly/io/async/EventBase.h>
#include <cstdint>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook { namespace fboss {

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
  virtual int readTransceiver(int dataAddress, int offset,
                              int len, uint8_t* fieldValue) = 0;
  virtual int writeTransceiver(int dataAddress, int offset,
                              int len, uint8_t* fieldValue) = 0;

  /*
   * This function will check if the transceiver is present or not
   */
  virtual bool detectTransceiver() = 0;

  /*
   * In an implementation where newly inserted transceivers needs to be cleared
   * out of reset. This is the function to do it.
   */
  virtual void ensureOutOfReset(){};

  /* This function does a hard reset of the QSFP and this will be
   * called when port flap is seen on the port remains down
   */
  virtual void triggerQsfpHardReset(){};

  /*
   * Returns the name of the port
   */
  virtual folly::StringPiece getName() = 0;

  virtual int getNum() const = 0;

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

 private:
  // Forbidden copy contructor and assignment operator
  TransceiverImpl(TransceiverImpl const &) = delete;
  TransceiverImpl& operator=(TransceiverImpl const &) = delete;

  enum : unsigned int { kMaxChannels = 4 };
};

}} // namespace facebook::fboss
