// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwAgentInitializer.h"

#ifndef IS_OSS
#if __has_feature(address_sanitizer)
#include <sanitizer/lsan_interface.h>
#endif
#endif

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

namespace {
/*
 * This function is executed periodically by the UpdateStats thread.
 * It calls the hardware-specific function of the same name.
 */
void updateStats(SwSwitch* swSwitch) {
  swSwitch->updateStats();
}
} // namespace

SwSwitchInitializer::SwSwitchInitializer(SwSwitch* sw) : sw_(sw) {}
SwSwitchInitializer::~SwSwitchInitializer() {
  if (initThread_) {
    initThread_->join();
    initThread_.reset();
  }
  fs_.reset();
}

void SwSwitchInitializer::start() {
  start(sw_);
}

void SwSwitchInitializer::start(HwSwitchCallback* callback) {
  initThread_ = std::make_unique<std::thread>(
      &SwSwitchInitializer::initThread, this, callback);
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
    init(callback);
  } catch (const std::exception& ex) {
    XLOG(FATAL) << "switch initialization failed: " << folly::exceptionStr(ex);
  }
}

void SwSwitchInitializer::init(HwSwitchCallback* hwSwitchCallback) {
  auto startTime = steady_clock::now();
  std::lock_guard<std::mutex> g(initLock_);
  initImpl(hwSwitchCallback);
  sw_->applyConfig("apply initial config");
  // Enable route update logging for all routes so that when we are told
  // the first set of routes after a warm boot, we can log any changes
  // from what was programmed before the warm boot.
  // e.g. any routes that were removed while the agent was restarting
  if (sw_->getBootType() == BootType::WARM_BOOT) {
    sw_->logRouteUpdates("::", 0, "fboss-agent-warmboot");
    sw_->logRouteUpdates("0.0.0.0", 0, "fboss-agent-warmboot");
  }
  sw_->initialConfigApplied(startTime);

  if (sw_->getBootType() == BootType::WARM_BOOT) {
    sw_->stopLoggingRouteUpdates("fboss-agent-warmboot");
  }
  // Start the UpdateSwitchStatsThread
  fs_ = std::make_unique<folly::FunctionScheduler>();
  fs_->setThreadName("UpdateStatsThread");
  // steady will help even out the interval which will especially make
  // aggregated counters more accurate with less spikes and dips
  fs_->setSteady(true);
  std::function<void()> callback(std::bind(updateStats, sw_));
  auto timeInterval = std::chrono::seconds(1);
  fs_->addFunction(callback, timeInterval, "updateStats");
  fs_->start();
  XLOG(DBG2) << "Started background thread: UpdateStatsThread";
  initCondition_.notify_all();
}

void SwAgentSignalHandler::signalReceived(int /*signum*/) noexcept {
  stopServices();
}

void SwAgentInitializer::stopServices() {
  // stop Thrift server: stop all worker threads and
  // stop accepting new connections
  XLOG(DBG2) << "Stopping thrift server";
  server_->stop();
  XLOG(DBG2) << "Stopped thrift server";
  initializer_->stopFunctionScheduler();
  XLOG(DBG2) << "Stopped stats FunctionScheduler";
  fbossFinalize();
}

void SwAgentInitializer::handleExitSignal() {
  restart_time::mark(RestartEvent::SIGNAL_RECEIVED);
  XLOG(DBG2) << "[Exit] Signal received ";
  steady_clock::time_point begin = steady_clock::now();
  stopServices();
  steady_clock::time_point servicesStopped = steady_clock::now();
  XLOG(DBG2) << "[Exit] Services stop time "
             << duration_cast<duration<float>>(servicesStopped - begin).count();
  sw_->gracefulExit();
  steady_clock::time_point switchGracefulExit = steady_clock::now();
  XLOG(DBG2)
      << "[Exit] Switch Graceful Exit time "
      << duration_cast<duration<float>>(switchGracefulExit - servicesStopped)
             .count()
      << std::endl
      << "[Exit] Total graceful Exit time "
      << duration_cast<duration<float>>(switchGracefulExit - begin).count();

  restart_time::mark(RestartEvent::SHUTDOWN);
  __attribute__((unused)) auto leakedSw = sw_.release();
#ifndef IS_OSS
#if __has_feature(address_sanitizer)
  __lsan_ignore_object(leakedSw);
#endif
#endif
  initializer_.reset();
}
} // namespace facebook::fboss
