/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/utils/PortFlapHelper.h"

namespace facebook::fboss::utility {

void PortFlapHelper::portFlap() {
  bool up = false;
  while (true) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait_for(lock, flapIntervalMs_, [this] { return done_.load(); });
      if (done_) {
        break;
      }
    }

    if (up) {
      ensemble_->bringUpPorts(portsToFlap_);
    } else {
      ensemble_->bringDownPorts(portsToFlap_);
    }
    up = !up;
  }
  ensemble_->bringUpPorts(portsToFlap_);
};

void PortFlapHelper::startPortFlap() {
  if (done_ == false) {
    throw FbossError("Port flap already started");
  }
  done_ = false;
  portFlapThread_ = std::thread(&PortFlapHelper::portFlap, this);
}

void PortFlapHelper::stopPortFlap() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    done_ = true;
    cv_.notify_one();
  }
  if (portFlapThread_.joinable()) {
    portFlapThread_.join();
  }
}
} // namespace facebook::fboss::utility
