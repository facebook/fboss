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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook { namespace fboss {

/* Virtual class to handle the different transceivers our equipment is likely
 * to support.  This supports, for now, QSFP and SFP.
 */

class TransceiverID;

class Transceiver {
 public:
  Transceiver() {}
  virtual ~Transceiver() {}

  /*
   * Transceiver type (SFP, QSFP)
   */
  virtual TransceiverType type() const = 0;

  virtual TransceiverID getID() const = 0;

  /*
   * Returns if the SFP is present or not
   */
  virtual bool isPresent() const = 0;

  /*
   * Check if the transceiver is present or not
   */
  virtual void detectTransceiver() = 0;

  /*
   * Return all of the transceiver information
   */
  virtual TransceiverInfo getTransceiverInfo() = 0;

  /*
   * Return raw page data from the qsfp DOM
   */
  virtual RawDOMData getRawDOMData() = 0;

  /*
   * Set speed specific settings for the transceiver
   */
  virtual void customizeTransceiver(cfg::PortSpeed speed) = 0;

  /*
   * Ensure QSFP settings are properly set.
   */
  virtual void customizeTransceiverIfDown() = 0;

  /*
   * Register that a logical port that is part of this transceiver has changed.
   */
  virtual void portChanged(uint32_t portID, PortStatus&& status) = 0;

 private:
  // no copy or assignment
  Transceiver(Transceiver const &) = delete;
  Transceiver& operator=(Transceiver const &) = delete;
};

}} //namespace facebook::fboss
