// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/single/MonolithicAgentInitializer.h"

#include <folly/MacAddress.h>
#include <folly/ScopeGuard.h>
#include <folly/SocketAddress.h>
#include <folly/String.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/FbossInit.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/RestartTimeTracker.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/single/MonolithicHwSwitchHandler.h"

#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/TunManager.h"
#include "fboss/lib/CommonUtils.h"

#include <gflags/gflags.h>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#ifndef IS_OSS
#if __has_feature(address_sanitizer)
#include <sanitizer/lsan_interface.h>
#endif
#endif

using apache::thrift::ThriftServer;
using folly::AsyncSignalHandler;
using folly::EventBase;
using folly::FunctionScheduler;
using folly::SocketAddress;
using std::condition_variable;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::chrono::seconds;
using namespace std::chrono;

DEFINE_int32(port, 5909, "The thrift server port");
// current default 5909 is in conflict with VNC ports, need to
// eventually migrate to 5959
DEFINE_int32(migrated_port, 5959, "New thrift server port migrate to");
DEFINE_int32(
    stat_publish_interval_ms,
    1000,
    "How frequently to publish thread-local stats back to the "
    "global store.  This should generally be less than 1 second.");
DEFINE_bool(
    tun_intf,
    true,
    "Create tun interfaces to allow other processes to "
    "send and receive traffic via the switch ports");
DEFINE_bool(enable_lacp, false, "Run LACP in agent");
DEFINE_bool(enable_lldp, true, "Run LLDP protocol in agent");
DEFINE_bool(publish_boot_type, true, "Publish boot type on startup");
// @lint-ignore CLANGTIDY
DECLARE_int32(thrift_idle_timeout);
DEFINE_bool(enable_macsec, false, "Enable Macsec functionality");

using facebook::fboss::SwSwitch;
using facebook::fboss::ThriftHandler;

namespace {
/*
 * This function is executed periodically by the UpdateStats thread.
 * It calls the hardware-specific function of the same name.
 */
void updateStats(SwSwitch* swSwitch) {
  swSwitch->updateStats();
}
} // namespace

namespace facebook::fboss {

void MonolithicSwSwitchInitializer::start() {
  start(sw_);
}

void MonolithicSwSwitchInitializer::start(HwSwitchCallback* callback) {
  std::thread t(&MonolithicSwSwitchInitializer::initThread, this, callback);
  // @lint-ignore CLANGTIDY
  t.detach();
}

void MonolithicSwSwitchInitializer::stopFunctionScheduler() {
  std::unique_lock<std::mutex> lk(initLock_);
  initCondition_.wait(lk, [&] { return sw_->isFullyInitialized(); });
  if (fs_) {
    fs_->shutdown();
  }
}

void MonolithicSwSwitchInitializer::waitForInitDone() {
  std::unique_lock<std::mutex> lk(initLock_);
  initCondition_.wait(lk, [&] { return sw_->isFullyInitialized(); });
}

void MonolithicSwSwitchInitializer::initThread(HwSwitchCallback* callback) {
  try {
    initImpl(callback);
  } catch (const std::exception& ex) {
    XLOG(FATAL) << "switch initialization failed: " << folly::exceptionStr(ex);
  }
}

SwitchFlags MonolithicSwSwitchInitializer::setupFlags() {
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

void MonolithicSwSwitchInitializer::initImpl(
    HwSwitchCallback* hwSwitchCallback) {
  auto startTime = steady_clock::now();
  std::lock_guard<mutex> g(initLock_);
  // Initialize the switch.  This operation can take close to a minute
  // on some of our current platforms.
  sw_->init(
      hwSwitchCallback,
      nullptr,
      [this](HwSwitchCallback* callback, bool failHwCallsOnWarmboot) {
        return hwAgent_->initAgent(failHwCallsOnWarmboot, callback);
      },
      setupFlags());

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
  fs_ = std::make_unique<FunctionScheduler>();
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

void MonolithicAgentSignalHandler::signalReceived(int /*signum*/) noexcept {
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
#ifndef IS_OSS
#if __has_feature(address_sanitizer)
  __lsan_ignore_object(sw_);
#endif
#endif

  exit(0);
}

MonolithicAgentInitializer::MonolithicAgentInitializer(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform) {
  createSwitch(std::move(config), hwFeaturesDesired, initPlatform);
}

void MonolithicAgentInitializer::createSwitch(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform) {
  // Allow any flag overrides to kick in before we create SwSwitch
  setCmdLineFlagOverrides();

  hwAgent_ = std::make_unique<HwAgent>(
      std::move(config), hwFeaturesDesired, initPlatform, 0 /*switchIndex*/);

  auto platform = hwAgent_->getPlatform();
  CHECK(platform);
  auto hwSwitchHandler =
      std::make_unique<MonolinithicHwSwitchHandler>(platform);

  // Create the SwSwitch and thrift handler
  sw_ = std::make_unique<SwSwitch>(std::move(hwSwitchHandler));
  initializer_ = std::make_unique<MonolithicSwSwitchInitializer>(
      sw_.get(), hwAgent_.get());
}

int MonolithicAgentInitializer::initAgent() {
  return initAgent(sw_.get());
}

int MonolithicAgentInitializer::initAgent(HwSwitchCallback* callback) {
  std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
      handlers{};
  auto swHandler = std::make_shared<ThriftHandler>(sw_.get());
  handlers.push_back(swHandler);
  auto hwHandler = platform()->createHandler();
  if (hwHandler) {
    handlers.push_back(hwHandler);
  }
  swHandler->setIdleTimeout(FLAGS_thrift_idle_timeout);
  eventBase_ = new EventBase();

  // Start the thrift server
  server_ = setupThriftServer(
      *eventBase_,
      handlers,
      {FLAGS_port, FLAGS_migrated_port},
      true /*setupSSL*/);

  swHandler->setSSLPolicy(server_->getSSLPolicy());

  // At this point, we are guaranteed no other agent process will initialize
  // the ASIC because such a process would have crashed attempting to bind to
  // the Thrift port 5909
  initializer_->start(callback);

  /*
   * Updating stats could be expensive as each update must acquire lock. To
   * avoid this overhead, we use ThreadLocal version for updating stats, and
   * start a publish thread to aggregate the counters periodically.
   */
  facebook::fb303::ThreadCachedServiceData::get()->startPublishThread(
      std::chrono::milliseconds(FLAGS_stat_publish_interval_ms));

  MonolithicAgentSignalHandler signalHandler(
      eventBase_, sw_.get(), [this]() { stopServices(); });

  XLOG(DBG2) << "serving on localhost on port " << FLAGS_port << " and "
             << FLAGS_migrated_port;
  // @lint-ignore CLANGTIDY
  server_->serve();
  return 0;
}

void MonolithicAgentInitializer::stopServices() {
  // stop Thrift server: stop all worker threads and
  // stop accepting new connections
  XLOG(DBG2) << "Stopping thrift server";
  server_->stop();
  XLOG(DBG2) << "Stopped thrift server";
  initializer_->stopFunctionScheduler();
  XLOG(DBG2) << "Stopped stats FunctionScheduler";
  fbossFinalize();
}

void MonolithicAgentInitializer::stopAgent(bool setupWarmboot) {
  stopServices();
  if (setupWarmboot) {
    sw_->gracefulExit();
    __attribute__((unused)) auto leakedSw = sw_.release();
    __attribute__((unused)) auto leakedHwAgent = hwAgent_.release();
#ifndef IS_OSS
#if __has_feature(address_sanitizer)
    __lsan_ignore_object(leakedSw);
    __lsan_ignore_object(leakedHwAgent);
#endif
#endif
  } else {
    auto revertToMinAlpmState =
        sw_->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
            HwAsic::Feature::ROUTE_PROGRAMMING);
    sw_->stop(revertToMinAlpmState);
  }
}

} // namespace facebook::fboss
