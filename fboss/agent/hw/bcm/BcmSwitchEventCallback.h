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
#include <string>

extern "C" {
#include <bcm/switch.h>
}

namespace facebook::fboss {

/**
 * This abstract class defines the interface for a Bcm switch event callback.
 */

class BcmSwitchEventCallback {
 public:
  BcmSwitchEventCallback() {}
  virtual ~BcmSwitchEventCallback() {}

  // override this function in derived classes to specify error handling
  // behavior (eg. logging a fatal error and terminating the program).
  virtual void callback(
      const int unit,
      const bcm_switch_event_t eventID,
      const uint32_t arg1,
      const uint32_t arg2,
      const uint32_t arg3,
      void* data) = 0;

 private:
  // disable copy constructor and assignment operator
  BcmSwitchEventCallback(const BcmSwitchEventCallback&) = delete;
  BcmSwitchEventCallback& operator=(const BcmSwitchEventCallback&) = delete;
};

/**
 * Handler for BCM unit errors.  All of these are fatal.
 */
class BcmSwitchEventUnitFatalErrorCallback : public BcmSwitchEventCallback {
 public:
  BcmSwitchEventUnitFatalErrorCallback() {}
  ~BcmSwitchEventUnitFatalErrorCallback() override {}

  void callback(
      const int unit,
      const bcm_switch_event_t eventID,
      const uint32_t arg1,
      const uint32_t arg2,
      const uint32_t arg3,
      void* data) override;
};

/**
 * Handler for non-fatal BCM unit errors. These should just log (and
 * potentially count) the errors.
 */
class BcmSwitchEventUnitNonFatalErrorCallback : public BcmSwitchEventCallback {
 public:
  BcmSwitchEventUnitNonFatalErrorCallback() {}
  ~BcmSwitchEventUnitNonFatalErrorCallback() override {}

  void callback(
      const int unit,
      const bcm_switch_event_t eventID,
      const uint32_t arg1,
      const uint32_t arg2,
      const uint32_t arg3,
      void* data) override;

 private:
  void logNonFatalError(
      int unit,
      const std::string& alarm,
      bcm_switch_event_t eventID,
      uint32_t arg1,
      uint32_t arg2,
      uint32_t arg3);
};

} // namespace facebook::fboss
