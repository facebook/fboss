/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Main.h"

#include <fb303/ServiceData.h>
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
#include "fboss/agent/FbossInit.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/RestartTimeTracker.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

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

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

namespace facebook::fboss {

void Initializer::start() {
  start(sw_);
}

void Initializer::start(HwSwitchCallback* callback) {
  std::thread t(&Initializer::initThread, this, callback);
  t.detach();
}

void Initializer::stopFunctionScheduler() {
  std::unique_lock<std::mutex> lk(initLock_);
  initCondition_.wait(lk, [&] { return sw_->isFullyInitialized(); });
  if (fs_) {
    fs_->shutdown();
  }
}

void Initializer::waitForInitDone() {
  std::unique_lock<std::mutex> lk(initLock_);
  initCondition_.wait(lk, [&] { return sw_->isFullyInitialized(); });
}

void Initializer::initThread(HwSwitchCallback* callback) {
  try {
    initImpl(callback);
  } catch (const std::exception& ex) {
    XLOG(FATAL) << "switch initialization failed: " << folly::exceptionStr(ex);
  }
}

SwitchFlags Initializer::setupFlags() {
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

void Initializer::initImpl(HwSwitchCallback* hwSwitchCallback) {
  auto startTime = steady_clock::now();
  std::lock_guard<mutex> g(initLock_);
  // Initialize the switch.  This operation can take close to a minute
  // on some of our current platforms.
  sw_->init(hwSwitchCallback, nullptr, setupFlags());

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

void SignalHandler::signalReceived(int /*signum*/) noexcept {
  restart_time::mark(RestartEvent::SIGNAL_RECEIVED);

  XLOG(DBG2) << "[Exit] Signal received ";
  steady_clock::time_point begin = steady_clock::now();
  stopServices_();
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

std::unique_ptr<AgentConfig> parseConfig(int argc, char** argv) {
  // one pass over flags, but don't clear argc/argv. We only do this
  // to extract the 'config' arg.
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  return AgentConfig::fromDefaultFile();
}

void initFlagDefaults(const std::map<std::string, std::string>& defaults) {
  for (auto item : defaults) {
    // logging not initialized yet, need to use std::cerr
    std::cerr << "Overriding default flag from config: " << item.first.c_str()
              << "=" << item.second.c_str() << std::endl;
    gflags::SetCommandLineOptionWithMode(
        item.first.c_str(), item.second.c_str(), gflags::SET_FLAGS_DEFAULT);
  }
}

void AgentInitializer::createSwitch(
    int argc,
    char** argv,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform) {
  setVersionInfo();

  // Read the config and set default command line arguments
  auto config = parseConfig(argc, argv);
  initFlagDefaults(*config->thrift.defaultCommandLineArgs());

  fbossInit(argc, argv);
  // Allow any flag overrides to kick in before we create SwSwitch
  setCmdLineFlagOverrides();
  // Allow the fb303 setOption() call to update the command line flag
  // settings.  This allows us to change the log levels on the fly using
  // setOption().
  fb303::fbData->setUseOptionsAsFlags(true);

  // Redirect stdin to /dev/null. This is really a extra precaution
  // we already disallow access to linux shell as a result of
  // executing thrift calls. Redirecting to /dev/null is done so that
  // if somehow a client did manage to get into the shell, the shell
  // would read EOF immediately and exit.
  if (!freopen("/dev/null", "r", stdin)) {
    XLOG(DBG2) << "Could not open /dev/null ";
  }

  // Initialize Bitsflow for agent
  initializeBitsflow();

  // Now that we have parsed the command line flags, create the Platform
  // object
  unique_ptr<Platform> platform =
      initPlatform(std::move(config), hwFeaturesDesired);

  // Create the SwSwitch and thrift handler
  sw_ = std::make_unique<SwSwitch>(std::move(platform));
  initializer_ =
      std::make_unique<Initializer>(sw_.get(), sw_->getPlatform_DEPRECATED());
}

int AgentInitializer::initAgent() {
  return initAgent(sw_.get());
}

int AgentInitializer::initAgent(HwSwitchCallback* callback) {
  auto handler =
      std::shared_ptr<ThriftHandler>(platform()->createHandler(sw_.get()));
  handler->setIdleTimeout(FLAGS_thrift_idle_timeout);
  eventBase_ = new EventBase();

  // Start the thrift server
  server_ = setupThriftServer(
      *eventBase_,
      handler,
      {FLAGS_port, FLAGS_migrated_port},
      true /*setupSSL*/);

  handler->setSSLPolicy(server_->getSSLPolicy());

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

  SignalHandler signalHandler(
      eventBase_, sw_.get(), [this]() { stopServices(); });

  XLOG(DBG2) << "serving on localhost on port " << FLAGS_port << " and "
             << FLAGS_migrated_port;
  server_->serve();
  return 0;
}

void AgentInitializer::stopServices() {
  // stop Thrift server: stop all worker threads and
  // stop accepting new connections
  XLOG(DBG2) << "Stopping thrift server";
  server_->stop();
  XLOG(DBG2) << "Stopped thrift server";
  initializer_->stopFunctionScheduler();
  XLOG(DBG2) << "Stopped stats FunctionScheduler";
  fbossFinalize();
}

void AgentInitializer::stopAgent(bool setupWarmboot) {
  stopServices();
  if (setupWarmboot) {
    sw_->gracefulExit();
    __attribute__((unused)) auto leakedSw = sw_.release();
#ifndef IS_OSS
#if __has_feature(address_sanitizer)
    __lsan_ignore_object(leakedSw);
#endif
#endif
  } else {
    auto revertToMinAlpmState =
        sw_->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
            HwAsic::Feature::ROUTE_PROGRAMMING);
    sw_->stop(revertToMinAlpmState);
  }
}

int fbossMain(
    int argc,
    char** argv,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform) {
  AgentInitializer initializer;
  initializer.createSwitch(argc, argv, hwFeaturesDesired, initPlatform);
  return initializer.initAgent();
}

} // namespace facebook::fboss
