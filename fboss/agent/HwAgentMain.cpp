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
#include "fboss/agent/mnpu/SplitAgentHwSwitchCallbackHandler.h"

namespace facebook::fboss {

int hwAgentMain(
    int argc,
    char** argv,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatformFn) {
  auto config = fbossCommonInit(argc, argv);

  auto hwAgent = std::make_unique<HwAgent>(
      std::move(config), hwFeaturesDesired, initPlatformFn);

  auto hwSwitchCallbackHandler =
      std::make_unique<SplitAgentHwSwitchCallbackHandler>();

  hwAgent->initAgent(
      true /* failHwCallsOnWarmboot */, hwSwitchCallbackHandler.get());

  hwAgent->start();
  return 0;
}

} // namespace facebook::fboss
