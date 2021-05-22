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
#include "fboss/agent/test/AgentTest.h"

DECLARE_string(config);

namespace facebook::fboss {

class MultiNodeTest : public AgentTest {
 protected:
  void SetUp() override;
  void TearDown() override;
  void setupFlags() override;
  void setupConfigFlag() override;

  std::unique_ptr<FbossCtrlAsyncClient> getRemoteThriftClient();

  void checkForRemoteSideRun();

 private:
  virtual cfg::SwitchConfig initialConfig() const = 0;
};
int mulitNodeTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);
} // namespace facebook::fboss
