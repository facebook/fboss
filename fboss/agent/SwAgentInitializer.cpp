// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwAgentInitializer.h"
#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/io/async/EventBase.h>

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
DEFINE_bool(enable_stats_update_thread, true, "Run stats update thread");

DEFINE_int32(port, 5909, "The thrift server port");
// current default 5909 is in conflict with VNC ports, need to
// eventually migrate to 5959
DEFINE_int32(migrated_port, 5959, "New thrift server port migrate to");
// @lint-ignore CLANGTIDY
DECLARE_int32(thrift_idle_timeout);
DEFINE_int32(
    sw_thrift_server_stop_timeout_s,
    10,
    "Seconds to wait for thrift server to stop.");

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

void SwSwitchInitializer::start(
    HwSwitchCallback* callback,
    const HwWriteBehavior& hwWriteBehavior) {
  initThread_ = std::make_unique<std::thread>(
      &SwSwitchInitializer::initThread, this, callback, hwWriteBehavior);
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
  if (fs_) {
    fs_->shutdown();
  }
}

void SwSwitchInitializer::waitForInitDone() {
  std::unique_lock<std::mutex> lk(initLock_);
  initCondition_.wait(lk, [&] { return sw_->isFullyConfigured(); });
}

void SwSwitchInitializer::initThread(
    HwSwitchCallback* callback,
    const HwWriteBehavior& hwWriteBehavior) {
  try {
    init(callback, hwWriteBehavior);
  } catch (const std::exception& ex) {
    XLOG(FATAL) << "switch initialization failed: " << folly::exceptionStr(ex);
  }
}

void SwSwitchInitializer::init(
    HwSwitchCallback* hwSwitchCallback,
    const HwWriteBehavior& hwWriteBehavior) {
  auto startTime = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> g(initLock_);
  initImpl(hwSwitchCallback, hwWriteBehavior);
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
  if (FLAGS_enable_stats_update_thread) {
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
  }
  initCondition_.notify_all();
}

void SwAgentSignalHandler::signalReceived(int /*signum*/) noexcept {
  stopServices();
}

void SwAgentInitializer::stopServer() {
  // Server init happens asynchronously, so if AgentEnsemble somehow exited
  // before server_ is set (e.g. due to an error), the code below will crash.
  if (!serverStarted_) {
    return;
  }
  // stop Thrift server: stop all worker threads and
  // stop accepting new connections
  XLOG(DBG2) << "Stop listening on thrift server";
  server_->stopListening();
  XLOG(DBG2) << "Stopping thrift server";
  auto stopController = server_->getStopController();
  if (auto lockedPtr = stopController.lock()) {
    lockedPtr->stop();
    XLOG(DBG2) << "Stopped thrift server";
    clearThriftModules();
    XLOG(DBG2) << "Cleared thrift modules";
  } else {
    LOG(WARNING) << "Unable to stop Thrift Server";
  }
}

void SwAgentInitializer::stopServices() {
  stopServer();
  initializer_->stopFunctionScheduler();
  XLOG(DBG2) << "Stopped stats FunctionScheduler";
  fbossFinalize();
}

void SwAgentInitializer::stopAgent(bool setupWarmboot, bool gracefulExit) {
  if (setupWarmboot) {
    handleExitSignal(gracefulExit);
  } else {
    stopServices();
    auto revertToMinAlpmState =
        sw_->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
            HwAsic::Feature::ROUTE_PROGRAMMING);
    sw_->stop(false /* gracefulStop */, revertToMinAlpmState);
    initializer_.reset();
  }
}

void SwAgentInitializer::waitForServerStopped() {
  if (serverStarted_) {
    std::unique_lock<std::mutex> lock(serverStopMutex_);
    serverStopCV_.wait_for(
        lock, std::chrono::seconds(FLAGS_sw_thrift_server_stop_timeout_s), [&] {
          return !serverStarted_;
        });
    if (serverStarted_) {
      XLOG(WARNING) << "Thrift server failed to stop after waiting "
                    << FLAGS_sw_thrift_server_stop_timeout_s << " seconds";
    }
  }
}

int SwAgentInitializer::initAgent() {
  CHECK_NE(sw_.get() != nullptr, (false));
  CHECK_NE((initializer_.get() != nullptr), (false));
  auto hwWriteBehavior = HwWriteBehavior::WRITE;
  if (sw_->getBootType() == BootType::WARM_BOOT) {
    hwWriteBehavior = HwWriteBehavior::LOG_FAIL;
  }
  return initAgent(sw_.get(), hwWriteBehavior);
}

int SwAgentInitializer::initAgent(
    HwSwitchCallback* callback,
    const HwWriteBehavior& hwWriteBehavior) {
  auto swHandler = std::make_shared<ThriftHandler>(sw_.get());
  swHandler->setIdleTimeout(FLAGS_thrift_idle_timeout);
  auto handlers = getThrifthandlers();
  handlers.push_back(swHandler);
  eventBase_ = new FbossEventBase();
  std::vector<int> ports = {FLAGS_port, FLAGS_migrated_port};
  // serve on hw agent port in mono so that clients can access
  // hw agent apis on mono as well
  if (!sw_->isRunModeMultiSwitch()) {
    ports.push_back(FLAGS_hwagent_port_base);
  }

  // Start the thrift server
  server_ = setupThriftServer(*eventBase_, handlers, ports, true /*setupSSL*/);

  server_->setStreamExpireTime(std::chrono::milliseconds(0));
  server_->setIdleTimeout(std::chrono::milliseconds(0));

  swHandler->setSSLPolicy(server_->getSSLPolicy());

  // At this point, we are guaranteed no other agent process will initialize
  // the ASIC because such a process would have crashed attempting to bind to
  // the Thrift port 5909
  initializer_->start(callback, hwWriteBehavior);

  /*
   * Updating stats could be expensive as each update must acquire lock. To
   * avoid this overhead, we use ThreadLocal version for updating stats, and
   * start a publish thread to aggregate the counters periodically.
   */
  facebook::fb303::ThreadCachedServiceData::get()->startPublishThread(
      std::chrono::milliseconds(FLAGS_stat_publish_interval_ms));

  SwAgentSignalHandler signalHandler(
      eventBase_, sw_.get(), [this]() { handleExitSignal(true); });

  {
    std::unique_lock<std::mutex> lk(serverStopMutex_);
    serverStarted_ = true;
  }
  XLOG(DBG2) << "serving on localhost on port " << FLAGS_port << " and "
             << FLAGS_migrated_port;
  // @lint-ignore CLANGTIDY
  server_->serve();
  server_.reset();
  {
    std::unique_lock<std::mutex> lk(serverStopMutex_);
    serverStarted_ = false;
  }
  serverStopCV_.notify_one();
  return 0;
}
} // namespace facebook::fboss
