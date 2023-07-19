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
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/FbossInit.h"
#include "fboss/agent/HwAgent.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/mnpu/SplitAgentHwSwitchCallbackHandler.h"

DEFINE_int32(switchIndex, 0, "Switch Index for Asic");

DEFINE_int32(
    hwagent_port_base,
    5931,
    "The first thrift server port reserved for HwAgent");

namespace facebook::fboss {

int hwAgentMain(
    int argc,
    char** argv,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatformFn) {
  auto config = fbossCommonInit(argc, argv);

  auto hwAgent = std::make_unique<HwAgent>(
      std::move(config), hwFeaturesDesired, initPlatformFn, FLAGS_switchIndex);

  auto hwSwitchCallbackHandler =
      std::make_unique<SplitAgentHwSwitchCallbackHandler>();

  hwAgent->initAgent(
      true /* failHwCallsOnWarmboot */, hwSwitchCallbackHandler.get());

  folly::EventBase eventBase;
  auto server = setupThriftServer(
      eventBase,
      {hwAgent->getPlatform()->createHandler()},
      {FLAGS_hwagent_port_base + FLAGS_switchIndex},
      true /*setupSSL*/);

  // we are sharing thrift server setup with swswitch which uses legacy
  // thrift server framework. This will be removed once the shared code
  // is migrated to ServiceFramework.
  // @lint-ignore CLANGTIDY
  server->serve();
  return 0;
}

} // namespace facebook::fboss
