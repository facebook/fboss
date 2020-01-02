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

#include <folly/Range.h>
#include <atomic>
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

extern "C" {
#include <opennsl/error.h>
}

namespace folly {
struct dynamic;
}

namespace facebook::fboss {

class BcmWarmBootHelper;

class BcmUnit {
 public:
  BcmUnit(int deviceIndex, BcmPlatform* platform);
  ~BcmUnit();

  /*
   * Initialize this BcmUnit and do a warm boot
   */
  void warmBootAttach() {
    attach(true);
  }

  /*
   * Initialize this BcmUnit and do a cold boot
   */
  void coldBootAttach() {
    attach(false);
  }
  /*
   * Detach bcm unit and clean up unit data structures from SDK.
   * In normal operations, just calling the shutdown routines
   * on SDK is enough. But we also support a mode where user
   * can run multiple tests on ASIC w/o restarting the test
   * binary. This method is used in that mode to do additional
   * cleanup of data structures, to facilitate multiple initialization
   * of unit w/o restarting the binary.
   */
  void detachAndCleanupSDKUnit();

  /*
   * Flush warm boot state to disk,
   */
  void writeWarmBootState(const folly::dynamic& switchState);

  bool isAttached() const {
    return attached_.load(std::memory_order_acquire);
  }

  unsigned int getNumber() const {
    return unit_;
  }

  void rawRegisterWrite(uint16_t phyID, uint8_t reg, uint16_t data);

  /*
   * Set a caller-defined cookie that can be retrieved with getCookie().
   *
   * This is useful for use in Broadcom SDK callbacks that do not support
   * their own user cookie.  As long as the callback returns the unit ID,
   * the BcmUnit cookie can be retrived by calling
   * BcmAPI::getUnit(unitNum)->getCookie();
   *
   * Only a single cookie may be defined.  Generally the owner of the BcmUnit
   * should be the only one to set a cookie.
   *
   * getCookie() and setCookie() do not perform any locking internally,
   * so the caller must provide their own locking if it is needed.
   */
  void* getCookie() const {
    return cookie_;
  }
  void setCookie(void* cookie) {
    cookie_ = cookie;
  }
  BcmWarmBootHelper* warmBootHelper() {
    CHECK(platform_);
    return platform_->getWarmBootHelper();
  }

 private:
  void attach(bool warmBoot);
  int createHwUnit();
  // Forbidden copy constructor and assignment operator
  BcmUnit(BcmUnit const&) = delete;
  BcmUnit& operator=(BcmUnit const&) = delete;

  void registerCallbackVector();
  void bcmInit();

  int deviceIndex_{-1};
  BcmPlatform* platform_{nullptr};
  int unit_{-1};
  std::atomic<bool> attached_{false};
  void* cookie_{nullptr};
};

} // namespace facebook::fboss
