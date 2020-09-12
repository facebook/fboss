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
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/agent/AgentConfig.h"
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

#include <gflags/gflags.h>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <functional>
#include <future>
#include <mutex>
#include <string>

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
DEFINE_int32(
    flush_warmboot_cache_secs,
    60,
    "Seconds to wait before flushing warm boot cache");
DECLARE_int32(thrift_idle_timeout);
DEFINE_bool(
    enable_standalone_rib,
    false,
    "Place the RIB under the control of the RoutingInformationBase object");

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

class Initializer {
 public:
  Initializer(SwSwitch* sw, Platform* platform)
      : sw_(sw), platform_(platform) {}

  void start() {
    std::thread t(&Initializer::initThread, this);
    t.detach();
  }

  void stopFunctionScheduler() {
    std::unique_lock<mutex> lk(initLock_);
    initCondition_.wait(lk, [&] { return sw_->isFullyInitialized(); });
    if (fs_) {
      fs_->shutdown();
    }
  }

 private:
  void initThread() {
    try {
      initImpl();
    } catch (const std::exception& ex) {
      XLOG(FATAL) << "switch initialization failed: "
                  << folly::exceptionStr(ex);
    }
  }

  SwitchFlags setupFlags() {
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
    if (FLAGS_enable_standalone_rib) {
      flags |= SwitchFlags::ENABLE_STANDALONE_RIB;
    }
    return flags;
  }

  void initImpl() {
    auto startTime = steady_clock::now();
    std::lock_guard<mutex> g(initLock_);
    // Determining the local MAC address can also take a few seconds the first
    // time it is called, so perform this operation asynchronously, in parallel
    // with the switch initialization.
    auto ret =
        std::async(std::launch::async, &Platform::getLocalMac, platform_);

    // Initialize the switch.  This operation can take close to a minute
    // on some of our current platforms.
    sw_->init(nullptr, setupFlags());

    // Wait for the local MAC address to be available.
    ret.wait();
    auto localMac = ret.get();
    XLOG(INFO) << "local MAC is " << localMac;

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

    // Start the UpdateSwitchStatsThread
    fs_ = new FunctionScheduler();
    fs_->setThreadName("UpdateStatsThread");
    std::function<void()> callback(std::bind(updateStats, sw_));
    auto timeInterval = std::chrono::seconds(1);
    fs_->addFunction(callback, timeInterval, "updateStats");
    fs_->start();
    XLOG(INFO) << "Started background thread: UpdateStatsThread";
    initCondition_.notify_all();
  }

  SwSwitch* sw_;
  Platform* platform_;
  FunctionScheduler* fs_;
  mutex initLock_;
  condition_variable initCondition_;
};

/*
 */
class SignalHandler : public AsyncSignalHandler {
  typedef std::function<void()> StopServices;

 public:
  SignalHandler(EventBase* eventBase, SwSwitch* sw, StopServices stopServices)
      : AsyncSignalHandler(eventBase), sw_(sw), stopServices_(stopServices) {
    registerSignalHandler(SIGINT);
    registerSignalHandler(SIGTERM);
  }
  void signalReceived(int /*signum*/) noexcept override {
    restart_time::mark(RestartEvent::SIGNAL_RECEIVED);

    XLOG(INFO) << "[Exit] Signal received ";
    steady_clock::time_point begin = steady_clock::now();
    stopServices_();
    steady_clock::time_point servicesStopped = steady_clock::now();
    XLOG(INFO)
        << "[Exit] Services stop time "
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

    exit(0);
  }

 private:
  SwSwitch* sw_;
  StopServices stopServices_;
};

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

int fbossMain(
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

  // Now that we have parsed the command line flags, create the Platform object
  unique_ptr<Platform> platform =
      initPlatform(std::move(config), hwFeaturesDesired);

  // Create the SwSwitch and thrift handler
  SwSwitch sw(std::move(platform));
  auto platformPtr = sw.getPlatform();
  auto handler =
      std::shared_ptr<ThriftHandler>(platformPtr->createHandler(&sw));

  handler->setIdleTimeout(FLAGS_thrift_idle_timeout);
  EventBase eventBase;

  // Start the thrift server
  auto server = setupThriftServer(
      eventBase,
      handler,
      FLAGS_port,
      true /*isDuplex*/,
      true /*setupSSL*/,
      true /*isStreaming*/);

  handler->setSSLPolicy(server->getSSLPolicy());

  // Create an Initializer to initialize the switch in a background thread.
  Initializer init(&sw, platformPtr);

  // At this point, we are guaranteed no other agent process will initialize the
  // ASIC because such a process would have crashed attempting to bind to the
  // Thrift port 5909
  init.start();

  /*
   * Updating stats could be expensive as each update must acquire lock. To
   * avoid this overhead, we use ThreadLocal version for updating stats, and
   * start a publish thread to aggregate the counters periodically.
   */
  facebook::fb303::ThreadCachedServiceData::get()->startPublishThread(
      std::chrono::milliseconds(FLAGS_stat_publish_interval_ms));

  auto stopServices = [&]() {
    // stop accepting new connections
    server->stopListening();
    XLOG(INFO) << "Stopped thrift server listening";
    init.stopFunctionScheduler();
    XLOG(INFO) << "Stopped stats FunctionScheduler";
    fbossFinalize();
  };
  SignalHandler signalHandler(&eventBase, &sw, stopServices);

  SCOPE_EXIT {
    server->cleanUp();
  };
  XLOG(INFO) << "serving on localhost on port " << FLAGS_port;

  // Run the EventBase main loop
  eventBase.loopForever();

  return 0;
}

} // namespace facebook::fboss
