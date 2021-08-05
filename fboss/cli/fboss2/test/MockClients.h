// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

namespace facebook::fboss {
using namespace ::testing;

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
};
} // namespace facebook::fboss
