// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwAgentInitializer.h"

DEFINE_bool(
    tun_intf,
    true,
    "Create tun interfaces to allow other processes to "
    "send and receive traffic via the switch ports");
DEFINE_bool(enable_lacp, false, "Run LACP in agent");
DEFINE_bool(enable_lldp, true, "Run LLDP protocol in agent");
DEFINE_bool(publish_boot_type, true, "Publish boot type on startup");
DEFINE_bool(enable_macsec, false, "Enable Macsec functionality");

namespace facebook::fboss {

SwSwitchInitializer::SwSwitchInitializer(SwSwitch* sw) : sw_(sw) {}

void SwSwitchInitializer::start() {
  start(sw_);
}

void SwSwitchInitializer::start(HwSwitchCallback* callback) {
  std::thread t(&SwSwitchInitializer::initThread, this, callback);
  // @lint-ignore CLANGTIDY
  t.detach();
}

SwitchFlags SwSwitchInitializer::setupFlags() {
  SwitchFlags flags = SwitchFlags::DEFAULT;
  if (FLAGS_enable_lacp) {
    flags |= SwitchFlags::ENABLE_LACP;
  }
  if (FLAGS_tun_intf) {
    flags |= SwitchFlags::ENABLE_TUN;
  }
  if (FLAGS_enable_lldp) {
    flags |= SwitchFlags::ENABLE_LLDP;
  }
  if (FLAGS_publish_boot_type) {
    flags |= SwitchFlags::PUBLISH_STATS;
  }
  if (FLAGS_enable_macsec) {
    flags |= SwitchFlags::ENABLE_MACSEC;
  }
  return flags;
}

void SwSwitchInitializer::stopFunctionScheduler() {
  std::unique_lock<std::mutex> lk(initLock_);
  initCondition_.wait(lk, [&] { return sw_->isFullyInitialized(); });
  if (fs_) {
    fs_->shutdown();
  }
}

void SwSwitchInitializer::waitForInitDone() {
  std::unique_lock<std::mutex> lk(initLock_);
  initCondition_.wait(lk, [&] { return sw_->isFullyInitialized(); });
}

void SwSwitchInitializer::initThread(HwSwitchCallback* callback) {
  try {
    initImpl(callback);
  } catch (const std::exception& ex) {
    XLOG(FATAL) << "switch initialization failed: " << folly::exceptionStr(ex);
  }
}

} // namespace facebook::fboss
