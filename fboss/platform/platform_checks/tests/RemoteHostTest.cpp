/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/platform_checks/RemoteHost.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace facebook::fboss::platform::platform_checks;
using ::testing::ElementsAre;
using ::testing::IsSupersetOf;

namespace {

// Stands in for ssh by running the "remote" command with a local shell.
class LoopbackRemoteHost : public RemoteHost {
 public:
  LoopbackRemoteHost() : RemoteHost("loopback", Transport::SSH) {}

  std::vector<std::string> execArgv(const std::string& cmd) const override {
    return {"/bin/sh", "-c", cmd};
  }
};

} // namespace

TEST(RemoteHostTest, SshPassesCommandAsSingleArgument) {
  RemoteHost host("rsw1", RemoteHost::Transport::SSH);

  auto argv = host.execArgv("echo a b");

  EXPECT_EQ(argv.front(), "ssh");
  EXPECT_THAT(argv, IsSupersetOf({"BatchMode=yes"}));
  EXPECT_THAT(
      std::vector<std::string>(argv.end() - 2, argv.end()),
      ElementsAre("root@rsw1", "echo a b"));
}

TEST(RemoteHostTest, Sush2Exec) {
  RemoteHost host("rsw1-oob", RemoteHost::Transport::SUSH2);

  EXPECT_THAT(
      host.execArgv("uptime"),
      ElementsAre(
          "sush2",
          "-q",
          "--reason",
          "fixmyfboss",
          "root@rsw1-oob",
          "--",
          "uptime"));
}

TEST(RemoteHostTest, CopyArgvTargetsHostPath) {
  RemoteHost ssh("rsw1", RemoteHost::Transport::SSH, "admin");
  RemoteHost sush2("rsw1", RemoteHost::Transport::SUSH2);

  auto sshArgv = ssh.copyArgv("/local/bin", "/tmp/bin");
  auto sush2Argv = sush2.copyArgv("/local/bin", "/tmp/bin");

  EXPECT_EQ(sshArgv.front(), "scp");
  EXPECT_THAT(
      std::vector<std::string>(sshArgv.end() - 2, sshArgv.end()),
      ElementsAre("/local/bin", "admin@rsw1:/tmp/bin"));
  EXPECT_THAT(
      sush2Argv,
      ElementsAre(
          "suscp2",
          "-r",
          "--reason",
          "fixmyfboss",
          "/local/bin",
          "root@rsw1:/tmp/bin"));
}

TEST(RemoteHostTest, RunAndFileAccessGoThroughTransport) {
  LoopbackRemoteHost host;

  auto result = host.run("echo hi; exit 2");

  EXPECT_EQ(result.exitCode, 2);
  EXPECT_EQ(result.standardOut, "hi\n");
  EXPECT_FALSE(host.isLocal());
  EXPECT_TRUE(host.exists("/proc/self"));
  EXPECT_EQ(host.readFile("/nonexistent"), std::nullopt);
}
