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
#include "common/stats/ServiceData.h"
#include "thrift/lib/cpp/async/TAsyncTimeout.h"
#include "thrift/lib/cpp/async/TAsyncSignalHandler.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp/transport/TSocketAddress.h"

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
using apache::thrift::async::TAsyncSignalHandler;
using apache::thrift::async::TAsyncTimeout;
using apache::thrift::async::TEventBase;
using apache::thrift::transport::TSocketAddress;
using std::shared_ptr;
using std::unique_ptr;
using std::mutex;
using std::chrono::seconds;
using std::condition_variable;
using std::string;

DEFINE_string(config, "", "The path to the local JSON configuration file");
DEFINE_int32(port, 5909, "The thrift server port");

DEFINE_int32(stat_publish_interval_ms, 1000,
             "How frequently to publish thread-local stats back to the "
             "global store.  This should generally be less than 1 second.");

DEFINE_bool(tun_intf, true,
            "Create tun interfaces to allow other processes to "
            "send and receive traffic via the switch ports");

DEFINE_bool(enable_lldp, true,
            "Run LLDP protocol in agent");

DEFINE_bool(publish_boot_type, true,
            "Publish boot type on startup");

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
      flags |= SwitchFlags::PUBLISH_BOOTTYPE;
    }
    return flags;
  }

  void initImpl() {
    std::lock_guard<mutex> g(initLock_);
    // Determining the local MAC address can also take a few seconds the first
    // time it is called, so perform this operation asynchronously, in parallel
    // with the switch initialization.
    auto ret = std::async(std::launch::async,
                          &Platform::getLocalMac, platform_);

    // Initialize the switch.  This operation can take close to a minute
    // on some of our current platforms.
    sw_->init(setupFlags());

    // Wait for the local MAC address to be available.
    ret.wait();
    auto localMac = ret.get();
    LOG(INFO) << "local MAC is " << localMac;

    sw_->updateStateBlocking("apply initial config",
                             [this](const shared_ptr<SwitchState>& state) {
                               return this->applyConfig(state, FLAGS_config);
                             });
    sw_->initialConfigApplied();

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
        seconds(30)/*initial delay*/);

    // Sfp Detection Thread
    const string sfpDetect = "SfpDetection";
    auto sfpDetectFunc = [=]() {
      sw_->detectSfp();
    };
    fs_->addFunction(sfpDetectFunc, seconds(1), sfpDetect);

    // Sfp Detection Thread
    const string sfpDomCacheUpdate = "SfpDomCacheUpdate";
    auto sfpDomCacheUpdateFunc = [=]() {
      sw_->updateSfpDomFields();
    };
    // Call sfpDomCacheUpdate 15 seconds to get SFP monitor the
    // DOM values
    fs_->addFunction(sfpDomCacheUpdateFunc, seconds(15), sfpDomCacheUpdate,
        seconds(15));

    fs_->start();
    LOG(INFO) << "Started background thread: UpdateStatsThread";
    initCondition_.notify_all();
  }

  shared_ptr<SwitchState> applyConfig(
      const shared_ptr<SwitchState>& state,
      folly::StringPiece config) {
    if (!config.empty()) {
      LOG(INFO) << "Loading config from local config file " << config;
      return applyThriftConfigFile(state, config, platform_);
    }

    return applyThriftConfigDefault(state, platform_);
  }

  SwSwitch *sw_;
  Platform *platform_;
  FunctionScheduler *fs_;
  mutex initLock_;
  condition_variable initCondition_;
};

class StatsPublisher : public TAsyncTimeout {
 public:
  StatsPublisher(TEventBase* eventBase, SwSwitch* sw,
                 std::chrono::milliseconds interval)
    : TAsyncTimeout(eventBase),
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
class SignalHandler : public TAsyncSignalHandler {
  typedef std::function<void()> StopServices;
 public:
  SignalHandler(TEventBase* eventBase, SwSwitch* sw,
      StopServices stopServices) :
    TAsyncSignalHandler(eventBase), sw_(sw), stopServices_(stopServices) {
    registerSignalHandler(SIGINT);
    registerSignalHandler(SIGTERM);
  }
  virtual void signalReceived(int signum) noexcept override {
    stopServices_();
    sw_->gracefulExit();
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
  google::SetCommandLineOptionWithMode("minloglevel", "0",
                                       google::SET_FLAGS_DEFAULT);

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
  auto handler = std::shared_ptr<ThriftHandler>(
      std::move(platformPtr->createHandler(&sw)));

  // Create an Initializer to initialize the switch in a background thread.
  // This allows us to start the thrift server while initialization is still in
  // progress.
  Initializer init(&sw, platformPtr);
  init.start();

  TEventBase eventBase;

  // Create a timeout to call sw->publishStats() once every second.
  StatsPublisher statsPublisher(&eventBase, &sw,
                                std::chrono::milliseconds(
                                    FLAGS_stat_publish_interval_ms));
  statsPublisher.start();

  auto stopServices = [&]() {
    statsPublisher.cancelTimeout();
    init.stopFunctionScheduler();
  };
  SignalHandler signalHandler(&eventBase, &sw, stopServices);
  // Start the thrift server
  ThriftServer server;
  server.getEventBaseManager()->setEventBase(&eventBase, false);
  server.setInterface(std::move(handler));
  server.setDuplex(true);

  // When a thrift connection closes, we need to clean up the associated
  // callbacks.
  server.setServerEventHandler(handler);
  TSocketAddress address;
  address.setFromLocalPort(FLAGS_port);
  server.setAddress(address);
  server.setup();
  SCOPE_EXIT { server.cleanUp(); };
  LOG(INFO) << "serving on localhost on port " << FLAGS_port;

  // Run the TEventBase main loop
  eventBase.loopForever();

  return 0;
}

}} // facebook::fboss
