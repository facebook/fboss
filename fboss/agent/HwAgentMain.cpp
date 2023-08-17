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
#include "fboss/agent/RestartTimeTracker.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/mnpu/SplitAgentThriftSyncer.h"

#include <chrono>

DEFINE_int32(switchIndex, 0, "Switch Index for Asic");

DEFINE_int32(
    hwagent_port_base,
    5931,
    "The first thrift server port reserved for HwAgent");

DEFINE_int32(swswitch_port, 5959, "Port for SwSwitch");

using namespace std::chrono;

namespace facebook::fboss {

void SplitHwAgentSignalHandler::signalReceived(int /*signum*/) noexcept {
  restart_time::mark(RestartEvent::SIGNAL_RECEIVED);

  XLOG(DBG2) << "[Exit] Signal received ";
  steady_clock::time_point begin = steady_clock::now();
  stopServices();
  steady_clock::time_point servicesStopped = steady_clock::now();
  XLOG(DBG2) << "[Exit] Services stop time "
             << duration_cast<duration<float>>(servicesStopped - begin).count();
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
      hwAgent->getPlatform()->getHwSwitch(), FLAGS_swswitch_port);

  auto bootType =
      hwAgent->initAgent(true /* failHwCallsOnWarmboot */, thriftSyncer.get());

  restart_time::init(
      hwAgent->getPlatform()->getDirectoryUtil()->getWarmBootDir(),
      bootType == BootType::WARM_BOOT);

  folly::EventBase eventBase;
  auto server = setupThriftServer(
      eventBase,
      {hwAgent->getPlatform()->createHandler()},
      {FLAGS_hwagent_port_base + FLAGS_switchIndex},
      true /*setupSSL*/);

  SplitHwAgentSignalHandler signalHandler(&eventBase, []() {});

  restart_time::mark(RestartEvent::INITIALIZED);

  // we are sharing thrift server setup with swswitch which uses legacy
  // thrift server framework. This will be removed once the shared code
  // is migrated to ServiceFramework.
  // @lint-ignore CLANGTIDY
  server->serve();
  return 0;
}

} // namespace facebook::fboss
