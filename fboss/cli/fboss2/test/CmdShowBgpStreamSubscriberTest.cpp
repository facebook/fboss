/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string_view>

#include "fboss/cli/fboss2/commands/show/bgp/stream/subscriber/CmdShowBgpStreamSubscriberPostPolicy.h"
#include "fboss/cli/fboss2/commands/show/bgp/stream/subscriber/CmdShowBgpStreamSubscriberPrePolicy.h"
#include "fboss/cli/fboss2/test/CmdBgpTestUtils.h" // NOLINT(misc-include-cleaner)
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

using namespace ::testing;
using namespace facebook::neteng::fboss::bgp::thrift;
namespace facebook::fboss {
class CmdShowBgpStreamSubscriberTestFixture : public CmdHandlerTestBase {
 public:
  std::map<TIpPrefix, std::vector<TBgpPath>> sessions_;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
  }
};

// As the output for the subscriber command is the same as the
// neighbors policy outputs, we only need to make sure the right
// thrift call is being made in this unit test.
TEST_F(CmdShowBgpStreamSubscriberTestFixture, queryClientPrePolicy) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getSubscriberNetworkInfo(_, _, _));

  auto results =
      CmdShowBgpStreamSubscriberPrePolicy().queryClient(localhost(), {"1"}, {});
}

TEST_F(CmdShowBgpStreamSubscriberTestFixture, queryClientPostPolicy) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getSubscriberNetworkInfo(_, _, _));

  auto results = CmdShowBgpStreamSubscriberPostPolicy().queryClient(
      localhost(), {"1"}, {});
}
} // namespace facebook::fboss
