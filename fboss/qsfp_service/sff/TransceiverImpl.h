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
#include <folly/String.h>
#include "fboss/agent/types.h"
#include "fboss/agent/FbossError.h"

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
   * Returns the name of the port
   */
  virtual folly::StringPiece getName() = 0;

  virtual int getNum() const = 0;

 private:
  // Forbidden copy contructor and assignment operator
  TransceiverImpl(TransceiverImpl const &) = delete;
  TransceiverImpl& operator=(TransceiverImpl const &) = delete;

  enum : unsigned int { kMaxChannels = 4 };
};

}} // namespace facebook::fboss
