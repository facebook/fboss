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
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/RestartTimeTracker.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/platforms/common/PlatformMode.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/qsfp_service/lib/QsfpClient.h"

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
  std::thread t(&Initializer::initThread, this);
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

void Initializer::initThread() {
  try {
    initImpl();
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

void Initializer::initImpl() {
  auto startTime = steady_clock::now();
  std::lock_guard<mutex> g(initLock_);
  // Initialize the switch.  This operation can take close to a minute
  // on some of our current platforms.
  sw_->init(nullptr, setupFlags());

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
  XLOG(INFO) << "Started background thread: UpdateStatsThread";
  initCondition_.notify_all();
}

void SignalHandler::signalReceived(int /*signum*/) noexcept {
  restart_time::mark(RestartEvent::SIGNAL_RECEIVED);

  XLOG(INFO) << "[Exit] Signal received ";
  steady_clock::time_point begin = steady_clock::now();
  stopServices_();
  steady_clock::time_point servicesStopped = steady_clock::now();
  XLOG(INFO) << "[Exit] Services stop time "
             << duration_cast<duration<float>>(servicesStopped - begin).count();
  sw_->gracefulExit();
  steady_clock::time_point switchGracefulExit = steady_clock::now();
  XLOG(INFO)
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
  initFlagDefaults(*config->thrift.defaultCommandLineArgs_ref());

  fbossInit(argc, argv);

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
    XLOG(INFO) << "Could not open /dev/null ";
  }

  // Now that we have parsed the command line flags, create the Platform
  // object
  unique_ptr<Platform> platform =
      initPlatform(std::move(config), hwFeaturesDesired);
  preAgentInit(*platform);

  // Create the SwSwitch and thrift handler
  sw_ = std::make_unique<SwSwitch>(std::move(platform));
  initializer_ = std::make_unique<Initializer>(sw_.get(), sw_->getPlatform());
}

void AgentInitializer::waitForQsfpServiceImpl(
    uint32_t retries,
    std::chrono::duration<uint32_t, std::milli> msBetweenRetry,
    bool failHard) const {
  folly::ScopedEventBaseThread evbThread;
  auto checkStatus = [&evbThread]() {
    std::atomic<bool> isAlive{false};
    auto fut =
        QsfpClient::createClient(evbThread.getEventBase())
            .thenValue([](auto&& client) { return client->future_getStatus(); })
            .thenValue([&isAlive](auto status) {
              isAlive.store(status == facebook::fb303::cpp2::fb_status::ALIVE);
            })
            .thenError(
                folly::tag_t<std::exception>{}, [](const std::exception& e) {
                  XLOG(ERR)
                      << "Exception talking to qsfp_service: " << e.what();
                });
    fut.wait();
    return isAlive.load();
  };
  try {
    checkWithRetry(checkStatus, retries, msBetweenRetry);
  } catch (const FbossError& ex) {
    XLOG(ERR) << " Failed to wait for QsfpSvc: " << ex.what();
    if (failHard) {
      throw;
    }
  }
}

void AgentInitializer::waitForQsfpService(const Platform& platform) const {
  std::chrono::duration<uint32_t, std::milli> msBetweenRetry(500);
  uint32_t retries{0};
  bool failHard{false};
  switch (platform.getMode()) {
    case PlatformMode::WEDGE:
    case PlatformMode::WEDGE100:
    case PlatformMode::GALAXY_LC:
    case PlatformMode::GALAXY_FC:
    case PlatformMode::MINIPACK:
    case PlatformMode::YAMP:
    case PlatformMode::WEDGE400C:
    case PlatformMode::WEDGE400:
    case PlatformMode::FUJI:
      // TODO - add wait here as well?
      break;
    case PlatformMode::ELBERT:
    case PlatformMode::CLOUDRIPPER:
      retries = 200; // upto 100s
      // Qsfp svc a must to program ports
      failHard = true;
      break;
    // Fake, sim platforms - no qsfp svc
    case PlatformMode::FAKE_WEDGE:
    case PlatformMode::FAKE_WEDGE40:
    case PlatformMode::WEDGE400C_SIM:
      break;
  }
  if (retries) {
    waitForQsfpServiceImpl(retries, msBetweenRetry, failHard);
  }
}

int AgentInitializer::initAgent() {
  auto handler =
      std::shared_ptr<ThriftHandler>(platform()->createHandler(sw_.get()));
  handler->setIdleTimeout(FLAGS_thrift_idle_timeout);
  eventBase_ = new EventBase();

  // Start the thrift server
  server_ = setupThriftServer(
      *eventBase_,
      handler,
      FLAGS_port,
      true /*isDuplex*/,
      true /*setupSSL*/,
      true /*isStreaming*/);

  handler->setSSLPolicy(server_->getSSLPolicy());

  // At this point, we are guaranteed no other agent process will initialize
  // the ASIC because such a process would have crashed attempting to bind to
  // the Thrift port 5909
  initializer_->start();

  /*
   * Updating stats could be expensive as each update must acquire lock. To
   * avoid this overhead, we use ThreadLocal version for updating stats, and
   * start a publish thread to aggregate the counters periodically.
   */
  facebook::fb303::ThreadCachedServiceData::get()->startPublishThread(
      std::chrono::milliseconds(FLAGS_stat_publish_interval_ms));

  SignalHandler signalHandler(
      eventBase_, sw_.get(), [this]() { stopServices(); });

  XLOG(INFO) << "serving on localhost on port " << FLAGS_port;
  server_->serve();
  return 0;
}

void AgentInitializer::stopServices() {
  // stop accepting new connections
  server_->stopListening();
  // Calling stopListening() alone does not stop worker
  // threads for duplex servers. Reset the server.
  server_.reset();
  XLOG(INFO) << "Stopped thrift server listening";
  initializer_->stopFunctionScheduler();
  XLOG(INFO) << "Stopped stats FunctionScheduler";
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
    sw_->stop(true);
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
