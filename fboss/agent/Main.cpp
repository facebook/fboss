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

#include <folly/experimental/FunctionScheduler.h>
#include <folly/MacAddress.h>
#include <folly/ScopeGuard.h>
#include <folly/String.h>
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/TunManager.h"
#include "common/stats/ServiceData.h"
#include <folly/io/async/AsyncTimeout.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <folly/SocketAddress.h>

#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <future>
#include <functional>
#include <string>
#include <mutex>
#include <gflags/gflags.h>

using folly::FunctionScheduler;
using apache::thrift::ThriftServer;
using folly::AsyncSignalHandler;
using folly::AsyncTimeout;
using folly::EventBase;
using folly::SocketAddress;
using std::shared_ptr;
using std::unique_ptr;
using std::mutex;
using std::chrono::seconds;
using std::condition_variable;
using std::string;
using namespace std::chrono;

DEFINE_int32(port, 5909, "The thrift server port");
DEFINE_int32(stat_publish_interval_ms, 1000,
             "How frequently to publish thread-local stats back to the "
             "global store.  This should generally be less than 1 second.");
DEFINE_int32(thrift_idle_timeout, 60, "Thrift idle timeout in seconds.");
// Programming 16K routes can take 20+ seconds
DEFINE_int32(thrift_task_expire_timeout, 30,
    "Thrift task expire timeout in seconds.");
DEFINE_bool(tun_intf, true,
            "Create tun interfaces to allow other processes to "
            "send and receive traffic via the switch ports");
DEFINE_bool(enable_lldp, true,
            "Run LLDP protocol in agent");
DEFINE_bool(publish_boot_type, true,
            "Publish boot type on startup");
DEFINE_bool(enable_nhops_prober, true,
            "Enables prober for unresolved next hops");
DEFINE_int32(flush_warmboot_cache_secs, 60,
    "Seconds to wait before flushing warm boot cache");
using facebook::fboss::SwSwitch;
using facebook::fboss::ThriftHandler;

namespace facebook { namespace fboss {

/*
 * This function is executed periodically by the UpdateStats thread.
 * It calls the hardware-specific function of the same name.
 */
void updateStats(SwSwitch *swSwitch) {
  swSwitch->getHw()->updateStats(swSwitch->stats());
}

class Initializer {
 public:
  Initializer(SwSwitch* sw, Platform* platform)
    : sw_(sw),
      platform_(platform) {}

  void start() {
    std::thread t(&Initializer::initThread, this);
    t.detach();
  }

  void stopFunctionScheduler() {
    std::unique_lock<mutex> lk(initLock_);
    initCondition_.wait(lk, [&] { return sw_->isFullyInitialized();});
    if (fs_) {
      fs_->shutdown();
    }
  }

 private:
  void initThread() {
    try {
      initImpl();
    } catch (const std::exception& ex) {
      LOG(FATAL) << "switch initialization failed: " <<
        folly::exceptionStr(ex);
    }
  }

  SwitchFlags setupFlags() {
    SwitchFlags flags = SwitchFlags::DEFAULT;
    if (FLAGS_tun_intf) {
      flags |= SwitchFlags::ENABLE_TUN;
    }
    if (FLAGS_enable_lldp) {
      flags |=  SwitchFlags::ENABLE_LLDP;
    }
    if (FLAGS_publish_boot_type) {
      flags |= SwitchFlags::PUBLISH_STATS;
    }
    if (FLAGS_enable_nhops_prober) {
      flags |= SwitchFlags::ENABLE_NHOPS_PROBER;
    }
    return flags;
  }

  void initImpl() {
    auto startTime = steady_clock::now();
    std::lock_guard<mutex> g(initLock_);
    // Determining the local MAC address can also take a few seconds the first
    // time it is called, so perform this operation asynchronously, in parallel
    // with the switch initialization.
    auto ret = std::async(std::launch::async,
                          &Platform::getLocalMac, platform_);

    // Initialize the switch.  This operation can take close to a minute
    // on some of our current platforms.
    sw_->init(nullptr, setupFlags());

    // Wait for the local MAC address to be available.
    ret.wait();
    auto localMac = ret.get();
    LOG(INFO) << "local MAC is " << localMac;

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
    const string& nameID = "updateStats";
    fs_->addFunction(callback, timeInterval, nameID);
    // Schedule function to signal to SwSwitch that all
    // initial programming is now complete. We typically
    // do that at the end of syncFib call from BGP but
    // in case that does not arrive for 30 secs after
    // applying config use the below function.
    const string flushWarmboot = "flushWarmBoot";
    auto flushWarmbootFunc = [=]() {
      sw_->clearWarmBootCache();
      fs_->cancelFunction(flushWarmboot);
    };
    // Call flushWarmBootFunc 30 seconds after applying config
    fs_->addFunction(flushWarmbootFunc, seconds(1), flushWarmboot,
        seconds(FLAGS_flush_warmboot_cache_secs)/*initial delay*/);

    fs_->start();
    LOG(INFO) << "Started background thread: UpdateStatsThread";
    initCondition_.notify_all();
  }

  SwSwitch *sw_;
  Platform *platform_;
  FunctionScheduler *fs_;
  mutex initLock_;
  condition_variable initCondition_;
};

class StatsPublisher : public AsyncTimeout {
 public:
  StatsPublisher(EventBase* eventBase, SwSwitch* sw,
                 std::chrono::milliseconds interval)
    : AsyncTimeout(eventBase),
      sw_(sw),
      interval_(interval) {}

  void start() {
    scheduleTimeout(interval_);
  }

  void timeoutExpired() noexcept override {
    sw_->publishStats();
    scheduleTimeout(interval_);
  }

 private:
  SwSwitch* sw_{nullptr};
  std::chrono::milliseconds interval_;
};

/*
 */
class SignalHandler : public AsyncSignalHandler {
  typedef std::function<void()> StopServices;
 public:
  SignalHandler(EventBase* eventBase, SwSwitch* sw,
      StopServices stopServices) :
    AsyncSignalHandler(eventBase), sw_(sw), stopServices_(stopServices) {
    registerSignalHandler(SIGINT);
    registerSignalHandler(SIGTERM);
  }
  void signalReceived(int /*signum*/) noexcept override {
    LOG(INFO) <<"[Exit] Signal received ";
    steady_clock::time_point begin = steady_clock::now();
    stopServices_();
    steady_clock::time_point servicesStopped = steady_clock::now();
    LOG(INFO)
        << "[Exit] Services stop time "
        << duration_cast<duration<float>>(servicesStopped - begin).count();
    sw_->gracefulExit();
    steady_clock::time_point switchGracefulExit = steady_clock::now();
    LOG(INFO)
        << "[Exit] Switch Graceful Exit time "
        << duration_cast<duration<float>>(switchGracefulExit - servicesStopped)
               .count()
        << std::endl
        << "[Exit] Total graceful Exit time "
        << duration_cast<duration<float>>(switchGracefulExit - begin).count();

    exit(0);
  }
 private:
  SwSwitch* sw_;
  StopServices stopServices_;
};

int fbossMain(int argc, char** argv, PlatformInitFn initPlatform) {

  fbossInit(argc, argv);

  // Internally we use a modified version of gflags that only shows VLOG
  // messages if --minloglevel is set to 0.  We pretty much always want to see
  // VLOG messages, so set minloglevel to 0 by default, unless overridden on
  // the command line.
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  // Allow the fb303 setOption() call to update the command line flag
  // settings.  This allows us to change the log levels on the fly using
  // setOption().
  fbData->setUseOptionsAsFlags(true);

  // Redirect stdin to /dev/null. This is really a extra precaution
  // we already disallow access to linux shell as a result of
  // executing thrift calls. Redirecting to /dev/null is done so that
  // if somehow a client did manage to get into the shell, the shell
  // would read EOF immediately and exit.
  freopen("/dev/null", "r", stdin);

  // Now that we have parsed the command line flags, create the Platform object
  unique_ptr<Platform> platform = initPlatform();

  // Create the SwSwitch and thrift handler
  SwSwitch sw(std::move(platform));
  auto platformPtr = sw.getPlatform();
  auto handler =
      std::shared_ptr<ThriftHandler>(platformPtr->createHandler(&sw));

  EventBase eventBase;

  // Start the thrift server
  ThriftServer server;
  server.setTaskExpireTime(std::chrono::milliseconds(
        FLAGS_thrift_task_expire_timeout * 1000));
  server.getEventBaseManager()->setEventBase(&eventBase, false);
  server.setInterface(handler);
  server.setDuplex(true);
  handler->setEventBaseManager(server.getEventBaseManager());

  // When a thrift connection closes, we need to clean up the associated
  // callbacks.
  server.setServerEventHandler(handler);

  SocketAddress address;
  address.setFromLocalPort(FLAGS_port);
  server.setAddress(address);
  server.setIdleTimeout(std::chrono::seconds(FLAGS_thrift_idle_timeout));
  handler->setIdleTimeout(FLAGS_thrift_idle_timeout);
  server.setup();

  // Create an Initializer to initialize the switch in a background thread.
  Initializer init(&sw, platformPtr);

  // At this point, we are guaranteed no other agent process will initialize the
  // ASIC because such a process would have crashed attempting to bind to the
  // Thrift port 5909
  init.start();

  // Create a timeout to call sw->publishStats() once every second.
  StatsPublisher statsPublisher(
      &eventBase, &sw,
      std::chrono::milliseconds(FLAGS_stat_publish_interval_ms));
  statsPublisher.start();

  auto stopServices = [&]() {
    statsPublisher.cancelTimeout();
    init.stopFunctionScheduler();
    fbossFinalize();
  };
  SignalHandler signalHandler(&eventBase, &sw, stopServices);

  SCOPE_EXIT { server.cleanUp(); };
  LOG(INFO) << "serving on localhost on port " << FLAGS_port;

  // Run the EventBase main loop
  eventBase.loopForever();

  return 0;
}

}} // facebook::fboss
