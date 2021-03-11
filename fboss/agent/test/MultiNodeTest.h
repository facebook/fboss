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
#include "fboss/agent/test/TestUtils.h"

DECLARE_string(config);

namespace facebook::fboss {

class MultiNodeTest : public ::testing::Test, public AgentInitializer {
 protected:
  void SetUp() override;
  void TearDown() override;
  void setPortStatus(PortID port, bool up);
  std::unique_ptr<FbossCtrlAsyncClient> getRemoteThriftClient();
  bool waitForSwitchStateCondition(
      std::function<bool(const std::shared_ptr<SwitchState>&)> conditionFn,
      uint32_t retries = 10,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(1000));

 private:
  virtual cfg::SwitchConfig initialConfig() const = 0;
  std::unique_ptr<std::thread> asyncInitThread_{nullptr};
  void setupConfigFlag();
};
int mulitNodeTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);
} // namespace facebook::fboss
