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

#include "fboss/agent/hw/bcm/types.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {

class BcmFlexCounter {
 public:
  explicit BcmFlexCounter(int unit) : unit_(unit) {}
  BcmFlexCounter(int unit, uint32_t counterID)
      : unit_(unit), counterID_(counterID) {}
  virtual ~BcmFlexCounter() {
    destroy(unit_, counterID_);
  }

  static void destroy(int unit, uint32_t counterID);

  uint32_t getID() const {
    return counterID_;
  }

 protected:
  int unit_{0};
  uint32_t counterID_{0};

 private:
  // Forbidden copy constructor and assignment operator
  BcmFlexCounter(BcmFlexCounter const&) = delete;
  BcmFlexCounter& operator=(BcmFlexCounter const&) = delete;
};
} // namespace facebook::fboss
