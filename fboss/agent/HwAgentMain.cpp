/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/HwAgentMain.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/FbossInit.h"
#include "fboss/agent/HwAgent.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/RestartTimeTracker.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/mnpu/SplitAgentThriftSyncer.h"

#include <chrono>

DEFINE_int32(switchIndex, 0, "Switch Index for Asic");

DEFINE_int32(
    hwagent_port_base,
    5931,
    "The first thrift server port reserved for HwAgent");

DEFINE_int32(swswitch_port, 5959, "Port for SwSwitch");

DEFINE_bool(enable_stats_update_thread, true, "Run stats update thread");

using namespace std::chrono;

namespace {

/*
 * This function is executed periodically by the UpdateStats thread.
 */
void updateStats(
    facebook::fboss::HwSwitch* hw,
    facebook::fboss::SplitAgentThriftSyncer* syncer) {
  hw->updateStats();
  auto hwSwitchStats = hw->getHwSwitchStats();
  syncer->updateHwSwitchStats(std::move(hwSwitchStats));
}
} // namespace

namespace facebook::fboss {

void SplitHwAgentSignalHandler::signalReceived(int /*signum*/) noexcept {
  restart_time::mark(RestartEvent::SIGNAL_RECEIVED);
  XLOG(DBG2) << "[Exit] Signal received ";
  if (!hwAgent_->isInitialized()) {
    XLOG(WARNING)
        << "[Exit] Signal received before initializing hw switch, waiting for initialization to finish.";
    hwAgent_->waitForInitDone();
  }
  steady_clock::time_point begin = steady_clock::now();
  stopServices();
  steady_clock::time_point servicesStopped = steady_clock::now();
  XLOG(DBG2) << "[Exit] Services stop time "
             << duration_cast<duration<float>>(servicesStopped - begin).count();
  hwAgent_->getPlatform()->getHwSwitch()->gracefulExit(state::WarmbootState());
  steady_clock::time_point switchGracefulExit = steady_clock::now();
  XLOG(DBG2)
      << "[Exit] Switch Graceful Exit time "
      << duration_cast<duration<float>>(switchGracefulExit - servicesStopped)
             .count()
      << std::endl
      << "[Exit] Total graceful Exit time "
      << duration_cast<duration<float>>(switchGracefulExit - begin).count();
  restart_time::mark(RestartEvent::SHUTDOWN);
  exit(0);
}

int hwAgentMain(
    int argc,
    char** argv,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatformFn) {
  auto config = fbossCommonInit(argc, argv);

  auto hwAgent = std::make_unique<HwAgent>(
      std::move(config), hwFeaturesDesired, initPlatformFn, FLAGS_switchIndex);

  auto thriftSyncer = std::make_unique<SplitAgentThriftSyncer>(
      hwAgent->getPlatform()->getHwSwitch(),
      FLAGS_swswitch_port,
      SwitchID(*hwAgent->getPlatform()->getAsic()->getSwitchId()),
      FLAGS_switchIndex);

  auto ret =
      hwAgent->initAgent(true /* failHwCallsOnWarmboot */, thriftSyncer.get());

  restart_time::init(
      hwAgent->getPlatform()->getDirectoryUtil()->getWarmBootDir(),
      ret.bootType == BootType::WARM_BOOT);

  thriftSyncer->start();

  std::unique_ptr<folly::FunctionScheduler> fs{nullptr};
  if (FLAGS_enable_stats_update_thread) {
    // Start the UpdateSwitchStatsThread
    fs.reset(new folly::FunctionScheduler());
    fs->setThreadName("UpdateStatsThread");
    // steady will help even out the interval which will especially make
    // aggregated counters more accurate with less spikes and dips
    fs->setSteady(true);
    std::function<void()> callback(std::bind(
        updateStats,
        hwAgent->getPlatform()->getHwSwitch(),
        thriftSyncer.get()));
    auto timeInterval = std::chrono::seconds(1);
    fs->addFunction(callback, timeInterval, "updateStats");
    fs->start();
    XLOG(DBG2) << "Started background thread: UpdateStatsThread";
  }

  folly::EventBase eventBase;
  auto server = setupThriftServer(
      eventBase,
      {hwAgent->getPlatform()->createHandler()},
      {FLAGS_hwagent_port_base + FLAGS_switchIndex},
      true /*setupSSL*/);

  SplitHwAgentSignalHandler signalHandler(
      &eventBase,
      [&thriftSyncer, &fs]() {
        thriftSyncer->stop();
        if (fs) {
          fs->shutdown();
        }
      },
      hwAgent.get());

  restart_time::mark(RestartEvent::INITIALIZED);

  // we are sharing thrift server setup with swswitch which uses legacy
  // thrift server framework. This will be removed once the shared code
  // is migrated to ServiceFramework.
  // @lint-ignore CLANGTIDY
  server->serve();
  return 0;
}

} // namespace facebook::fboss
