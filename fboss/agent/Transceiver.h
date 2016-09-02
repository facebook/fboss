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
#include "fboss/agent/if/gen-cpp2/optic_types.h"

namespace facebook { namespace fboss {

/* Virtual class to handle the different transceivers our equipment is likely
 * to support.  This supports, for now, QSFP and SFP.
 */

class Transceiver {
 public:
  Transceiver() {}
  virtual ~Transceiver() {}

  /*
   * Transceiver type (SFP, QSFP)
   */
  virtual TransceiverType type() const = 0;
  /*
   * Returns if the SFP is present or not
   */
  virtual bool isPresent() const = 0;
  /*
   * Check if the transceiver is present or not
   */
  virtual void detectTransceiver() = 0;
  /*
   * Update the transceiver information in the cache
   */
  virtual void updateTransceiverInfoFields() = 0;
  /*
   * Return all of the transceiver information
   */
  virtual void getTransceiverInfo(TransceiverInfo &info) = 0;
  /*
   * Return SFP-specific information
   */
  virtual void getSfpDom(SfpDom &dom) = 0;

 private:
  // no copy or assignment
  Transceiver(Transceiver const &) = delete;
  Transceiver& operator=(Transceiver const &) = delete;
};

}} //namespace facebook::fboss
