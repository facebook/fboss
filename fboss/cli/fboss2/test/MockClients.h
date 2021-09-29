// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

using namespace ::testing;
namespace facebook::fboss {

extern std::vector<facebook::fboss::ArpEntryThrift> createArpEntries();

class MockFbossCtrlAgent : public FbossCtrlSvIf {
 public:
  MOCK_METHOD(void, getAclTable, (std::vector<AclEntryThrift>&));

  MOCK_METHOD(
      void,
      getArpTable,
      (std::vector<facebook::fboss::ArpEntryThrift>&));

  using PortInfoMap = std::map<int, facebook::fboss::PortInfoThrift>&;
  MOCK_METHOD(void, getAllPortInfo, (PortInfoMap));

  /* This unit test is a special case because the thrift spec for getRegexCounters
  uses "thread = eb".  This requires a pretty ugly mock definition and call to work */
  MOCK_METHOD2(
      async_eb_getRegexCounters,
      void(std::unique_ptr<apache::thrift::HandlerCallback<
              std::unique_ptr<std::map<std::string, int64_t>>>>,
          std::unique_ptr<std::string> regex));
};
} // namespace facebook::fboss
