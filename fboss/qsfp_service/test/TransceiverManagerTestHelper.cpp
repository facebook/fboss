/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/TransceiverManagerTestHelper.h"

#include "fboss/qsfp_service/test/FakeConfigsHelper.h"

namespace facebook::fboss {
void TransceiverManagerTestHelper::SetUp() {
  setupFakeAgentConfig(agentCfgPath);
  setupFakeQsfpConfig(qsfpCfgPath);

  // Set up fake qsfp_service volatile directory
  gflags::SetCommandLineOptionWithMode(
      "qsfp_service_volatile_dir",
      qsfpSvcVolatileDir.c_str(),
      gflags::SET_FLAGS_DEFAULT);

  gflags::SetCommandLineOptionWithMode(
      "qsfp_data_refresh_interval", "0", gflags::SET_FLAGS_DEFAULT);

  gflags::SetCommandLineOptionWithMode(
      "use_platform_mapping", "true", gflags::SET_FLAGS_DEFAULT);

  // Create a wedge manager
  transceiverManager_ =
      std::make_unique<MockWedgeManager>(numModules, 4 /* portsPerModule */);
  transceiverManager_->init();
}

void TransceiverManagerTestHelper::resetTransceiverManager() {
  transceiverManager_.reset();
  transceiverManager_ =
      std::make_unique<MockWedgeManager>(numModules, 4 /* portsPerModule */);
}
} // namespace facebook::fboss
