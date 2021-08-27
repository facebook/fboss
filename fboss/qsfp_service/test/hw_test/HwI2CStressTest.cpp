// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"

#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

#include <folly/logging/xlog.h>

namespace {
constexpr int kI2cStressTestIterations = 500;
}

namespace facebook::fboss {

TEST_F(HwTest, i2cStress) {
  auto agentConfig = getHwQsfpEnsemble()->getWedgeManager()->getAgentConfig();
  auto transceivers = utility::legacyTransceiverIds(
      utility::getCabledPortTranceivers(*agentConfig, getHwQsfpEnsemble()));
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  std::map<int32_t, TransceiverInfo> transceiversInfo;
  getHwQsfpEnsemble()->getWedgeManager()->getTransceiversInfo(
      transceiversInfo, std::make_unique<std::vector<int32_t>>(transceivers));
  std::map<int32_t, ReadResponse> previousResponse;

  ReadRequest request;
  request.ids_ref() = transceivers;
  TransceiverIOParameters params;
  params.offset_ref() =
      0; // Identifier Byte 0 in all transceivers remains constant across reads
  request.parameter_ref() = params;

  for (auto iteration = 1; iteration <= kI2cStressTestIterations; iteration++) {
    std::unique_ptr<ReadRequest> readRequest =
        std::make_unique<ReadRequest>(request);
    std::map<int32_t, ReadResponse> currentResponse;
    wedgeManager->readTransceiverRegister(
        currentResponse, std::move(readRequest));
    for (auto tcvrId : transceivers) {
      // Assert the following things
      // 1. Response is returned for all the requested transceiver IDs
      // 2. Response is valid
      // 3. Data read should be the same in all iterations
      // 4. Byte 0 is the module identifier and should match the one in
      // transceiversInfo
      EXPECT_TRUE(currentResponse.find(tcvrId) != currentResponse.end());
      auto curr = currentResponse[tcvrId];
      EXPECT_TRUE(*curr.valid_ref());
      TransceiverModuleIdentifier identifier =
          static_cast<TransceiverModuleIdentifier>(*(curr.data_ref()->data()));
      EXPECT_EQ(
          identifier, transceiversInfo[tcvrId].identifier_ref().value_or({}));
      EXPECT_TRUE(identifier != TransceiverModuleIdentifier::UNKNOWN);
      if (iteration != 1) {
        auto prev = previousResponse[tcvrId];
        EXPECT_EQ(*(prev.data_ref()->data()), *(curr.data_ref()->data()));
      }
    }
    previousResponse = currentResponse;
  }
}
} // namespace facebook::fboss
