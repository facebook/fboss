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

#include <boost/utility.hpp>

extern "C" {
#include <opennsl/switch.h>
}

namespace facebook { namespace fboss {

class BcmSwitchEvent;

/**
 * This abstract class defines the interface for a Bcm switch event callback.
 */

class BcmSwitchEventCallback : public boost::noncopyable {
 public:
  BcmSwitchEventCallback() {}
  virtual ~BcmSwitchEventCallback() {}

  // override this function in derived classes to specify error handling
  // behavior (eg. logging a fatal error and terminating the program).
  virtual void callback(const BcmSwitchEvent& event)=0;
};

/**
 * Default handler for Bcm parity errors.
 * Logs a descriptive fatal error and quits.
 */
class BcmSwitchEventParityErrorCallback : public BcmSwitchEventCallback {
 public:
  BcmSwitchEventParityErrorCallback() {}
  ~BcmSwitchEventParityErrorCallback() override {}

  void callback(const BcmSwitchEvent& event) override;
};

}} // facebook::fboss
