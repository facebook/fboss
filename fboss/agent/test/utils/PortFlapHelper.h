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
#include <thread>
#include <vector>
#include "fboss/agent/test/AgentEnsemble.h"

namespace facebook::fboss::utility {

class PortFlapHelper {
 public:
  PortFlapHelper(
      AgentEnsemble* ensemble,
      std::vector<PortID>& portsToFlap,
      int flapIntervalMs = 400)
      : portsToFlap_(portsToFlap) {
    flapIntervalMs_ = std::chrono::milliseconds(flapIntervalMs);
    ensemble_ = ensemble;
    portsToFlap_ = portsToFlap;
  }
  ~PortFlapHelper() {
    if (done_) {
      return;
    }
    stopPortFlap();
  }
  void startPortFlap();

  void stopPortFlap();

 private:
  void portFlap();
  std::thread portFlapThread_;
  std::atomic<bool> done_ = true;
  std::condition_variable cv_;
  std::chrono::milliseconds flapIntervalMs_;
  std::vector<PortID> portsToFlap_;
  AgentEnsemble* ensemble_;
  std::mutex mutex_;
};

} // namespace facebook::fboss::utility
