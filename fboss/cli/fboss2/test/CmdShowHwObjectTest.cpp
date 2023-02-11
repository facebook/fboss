// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/hwobject/CmdShowHwObject.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

utils::HwObjectList createHwObjectTypes() {
  utils::HwObjectList hwObjectTypes({"BUFFER", "DEBUG_COUNTER"});
  return hwObjectTypes;
}

std::string createExpectedHwObjectInfo() {
  return "Object type: buffer-pool\n"
         "BufferPoolSaiId(1202590842881): (Type: 1, Size: 14758016, ThresholdMode: 1, nullopt)\n\n"
         "Object type: buffer-profile\n"
         "BufferProfileSaiId(107374182405): (PoolId: 1202590842881, ReservedBytes: 0, ThresholdMode: 1, SharedDynamicThreshold: 1, XoffTh: 0, XonTh: 0, XonOffsetTh: 0)\n"
         "BufferProfileSaiId(107374182404): (PoolId: 1202590842881, ReservedBytes: 0, ThresholdMode: 1, SharedDynamicThreshold: 0, XoffTh: 0, XonTh: 0, XonOffsetTh: 0)\n"
         "BufferProfileSaiId(107374182403): (PoolId: 1202590842881, ReservedBytes: 9984, ThresholdMode: 1, SharedDynamicThreshold: 3, XoffTh: 0, XonTh: 0, XonOffsetTh: 0)\n"
         "BufferProfileSaiId(107374182402): (PoolId: 1202590842881, ReservedBytes: 3328, ThresholdMode: 1, SharedDynamicThreshold: 0, XoffTh: 0, XonTh: 0, XonOffsetTh: 0)\n"
         "BufferProfileSaiId(107374182401): (PoolId: 1202590842881, ReservedBytes: 1664, ThresholdMode: 1, SharedDynamicThreshold: 1, XoffTh: 0, XonTh: 0, XonOffsetTh: 0)\n\n"
         "Object type: debug-counter\n"
         "DebugCounterSaiId(365072220160): (Type: 0, BindMethod: 0, InDropReasons: [52])\n";
}

class CmdShowHwObjectTestFixture : public CmdHandlerTestBase {
 public:
  CmdShowHwObjectTraits::ObjectArgType queriedHwObjectTypes;
  utils::HwObjectList mockHwObjectTypes;
  std::string expectedHwObjectInfo;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockHwObjectTypes = createHwObjectTypes();
    expectedHwObjectInfo = createExpectedHwObjectInfo();
  }
};

TEST_F(CmdShowHwObjectTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), listHwObjects(_, _, _))
      .WillOnce(Invoke([&](std::string& out,
                           std::unique_ptr<std::vector<HwObjectType>>,
                           bool) { out = expectedHwObjectInfo; }));

  auto cmd = CmdShowHwObject();
  auto hwObjectInfo = cmd.queryClient(localhost(), mockHwObjectTypes);
  EXPECT_THRIFT_EQ(hwObjectInfo, expectedHwObjectInfo);
}

TEST_F(CmdShowHwObjectTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowHwObject().printOutput(expectedHwObjectInfo, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      "Object type: buffer-pool\n"
      "BufferPoolSaiId(1202590842881): (Type: 1, Size: 14758016, ThresholdMode: 1, nullopt)\n\n"
      "Object type: buffer-profile\n"
      "BufferProfileSaiId(107374182405): (PoolId: 1202590842881, ReservedBytes: 0, ThresholdMode: 1, SharedDynamicThreshold: 1, XoffTh: 0, XonTh: 0, XonOffsetTh: 0)\n"
      "BufferProfileSaiId(107374182404): (PoolId: 1202590842881, ReservedBytes: 0, ThresholdMode: 1, SharedDynamicThreshold: 0, XoffTh: 0, XonTh: 0, XonOffsetTh: 0)\n"
      "BufferProfileSaiId(107374182403): (PoolId: 1202590842881, ReservedBytes: 9984, ThresholdMode: 1, SharedDynamicThreshold: 3, XoffTh: 0, XonTh: 0, XonOffsetTh: 0)\n"
      "BufferProfileSaiId(107374182402): (PoolId: 1202590842881, ReservedBytes: 3328, ThresholdMode: 1, SharedDynamicThreshold: 0, XoffTh: 0, XonTh: 0, XonOffsetTh: 0)\n"
      "BufferProfileSaiId(107374182401): (PoolId: 1202590842881, ReservedBytes: 1664, ThresholdMode: 1, SharedDynamicThreshold: 1, XoffTh: 0, XonTh: 0, XonOffsetTh: 0)\n\n"
      "Object type: debug-counter\n"
      "DebugCounterSaiId(365072220160): (Type: 0, BindMethod: 0, InDropReasons: [52])\n\n";

  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
