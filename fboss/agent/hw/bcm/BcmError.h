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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

extern "C" {
#include <opennsl/pkt.h>
#include <opennsl/error.h>
#include <shared/error.h>
}

namespace facebook { namespace fboss {

/**
 * A class for errors from the Broadcom SDK.
 */
class BcmError : public FbossError {
 public:
  template<typename... Args>
  BcmError(int err, Args&&... args)
    : FbossError(std::forward<Args>(args)..., ": ", opennsl_errmsg(err)),
      err_(static_cast<opennsl_error_t>(err)) {}

  template<typename... Args>
  BcmError(opennsl_error_t err, Args&&... args)
    : FbossError(std::forward<Args>(args)..., ": ", opennsl_errmsg(err)),
      err_(err) {}

  ~BcmError() throw() override {}

  opennsl_error_t getBcmError() const {
    return err_;
  }

 private:
  opennsl_error_t err_;
};

template<typename... Args>
void bcmCheckError(int err, Args&&... msgArgs) {
  if (OPENNSL_FAILURE(err)) {
    throw BcmError(err, std::forward<Args>(msgArgs)...);
  }
}

template<typename... Args>
void bcmLogError(int err, Args&&... msgArgs) {
  if (OPENNSL_FAILURE(err)) {
    LOG(ERROR) << folly::to<std::string>(std::forward<Args>(msgArgs)...)
               << ": " << opennsl_errmsg(err) << ", " << err;
  }
}

template<typename... Args>
void bcmLogFatal(int err, const BcmSwitch* hw, Args&&... msgArgs) {
  if (OPENNSL_FAILURE(err)) {
    hw->exitFatal();
    LOG(FATAL) << folly::to<std::string>(std::forward<Args>(msgArgs)...)
               << ": " << opennsl_errmsg(err) << ", " << err;
  }
}

}} // facebook::fboss
