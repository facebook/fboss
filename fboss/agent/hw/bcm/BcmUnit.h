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

#include <atomic>
#include <folly/Range.h>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

extern "C" {
#include <opennsl/error.h>
}

namespace folly {
struct dynamic;
}

namespace facebook { namespace fboss {

class BcmWarmBootHelper;

class BcmUnit {
 public:
  explicit BcmUnit(int deviceIndex);
  ~BcmUnit();

  /*
   * Initialize this BcmUnit.
   *
   * The warmBootDir specifies the directory where switch state should be stored
   * to allow warm booting the switch the next time it is initialized.
   *
   * The contents of this directory will be used by the BcmWarmBootHelper to
   * determine if a warm boot should be performed. In order for warm boot to
   * work properly the directory should not change between runs.
   */
  void attach(std::string warmBootDir);

  /*
   * Initialize this BcmUnit, without warm boot support.
   *
   * The unit will perform a cold reset, and will not support warm boot
   * the next time it is initialized.
   */
  void attach();

  /*
   * Flush warm boot state to disk, and then detach from the hardware
   * device, without changing any hardware state.
   *
   * Once detach() has been called no other methods other than the
   * BcmUnit destructor should be invoked.
   */
  void detach(const std::string& switchStateFile,
      folly::dynamic& switchState);

  /*
   * Returns the boot type used when the unit was loaded. This will be either
   * WARM_BOOT or COLD_BOOT if the unit is attached. If the unit is not attached
   * it should return UNINITIALIZED.
   */
  BootType bootType();

  /**
   * Get the WarmBootHelper object.
   *
   * The NeighborUpdater returned is owned by the BcmUnit, and is only valid as
   * long as the BcmUnit object.
   */
  BcmWarmBootHelper* warmBootHelper() {
    return wbHelper_.get();
  }

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

 private:
  // Forbidden copy constructor and assignment operator
  BcmUnit(BcmUnit const &) = delete;
  BcmUnit& operator=(BcmUnit const &) = delete;

  void registerCallbackVector();
  void bcmInit();
  void onSwitchEvent(opennsl_switch_event_t event,
                     uint32_t arg1, uint32_t arg2, uint32_t arg3);

  static void switchEventCallback(int unit, opennsl_switch_event_t event,
                                  uint32_t arg1, uint32_t arg2, uint32_t arg3,
                                  void* userdata);

  std::unique_ptr<BcmWarmBootHelper> wbHelper_;
  int deviceIndex_{-1};
  int unit_{-1};
  std::atomic<bool> attached_{false};
  void* cookie_{nullptr};
};

}} // facebook::fboss
