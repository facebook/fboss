// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/experimental/FunctionScheduler.h>

#include "fboss/agent/SwSwitch.h"

#include <gflags/gflags.h>
#include <condition_variable>
#include <mutex>

DECLARE_bool(tun_intf);
DECLARE_bool(enable_lacp);
DECLARE_bool(enable_lldp);
DECLARE_bool(publish_boot_type);
DECLARE_bool(enable_macsec);

namespace facebook::fboss {

class SwSwitchInitializer {
 public:
  explicit SwSwitchInitializer(SwSwitch* sw);
  virtual ~SwSwitchInitializer() = default;
  void start();
  void start(HwSwitchCallback* callback);
  void stopFunctionScheduler();
  void waitForInitDone();

 protected:
  virtual void initImpl(HwSwitchCallback*) = 0;
  void initThread(HwSwitchCallback* callback);
  SwitchFlags setupFlags();

  SwSwitch* sw_;
  std::unique_ptr<folly::FunctionScheduler> fs_;
  std::mutex initLock_;
  std::condition_variable initCondition_;
};

} // namespace facebook::fboss
