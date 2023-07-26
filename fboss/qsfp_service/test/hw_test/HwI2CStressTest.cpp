// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/agent/platforms/common/PlatformMapping.h"

#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

#include <folly/gen/Base.h>
#include <folly/logging/xlog.h>

namespace {
constexpr int kI2cStressTestIterations = 200;
}

namespace facebook::fboss {

TEST_F(HwTest, i2cStressRead) {
  auto transceivers = utility::legacyTransceiverIds(
      utility::getCabledPortTranceivers(getHwQsfpEnsemble()));
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  std::map<int32_t, TransceiverInfo> transceiversInfo;
  getHwQsfpEnsemble()->getWedgeManager()->getTransceiversInfo(
      transceiversInfo, std::make_unique<std::vector<int32_t>>(transceivers));
  std::map<int32_t, ReadResponse> previousResponse;

  ReadRequest request;
  request.ids() = transceivers;
  TransceiverIOParameters params;
  params.offset() =
      0; // Identifier Byte 0 in all transceivers remains constant across reads
  request.parameter() = params;

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
      EXPECT_TRUE(*curr.valid())
          << "Invalid response on transceiver " << tcvrId;
      if (!curr.get_valid()) {
        // Don't access data if it's not valid.
        continue;
      }

      TransceiverModuleIdentifier identifier =
          static_cast<TransceiverModuleIdentifier>(*(curr.data()->data()));
      EXPECT_EQ(identifier, transceiversInfo[tcvrId].identifier().value_or({}));
      EXPECT_TRUE(identifier != TransceiverModuleIdentifier::UNKNOWN);
      if (iteration != 1) {
        auto prev = previousResponse[tcvrId];
        if (prev.get_valid()) {
          EXPECT_EQ(*(prev.data()->data()), *(curr.data()->data()));
        }
      }
    }
    previousResponse = currentResponse;
  }
}

TEST_F(HwTest, i2cStressWrite) {
  auto transceivers = utility::legacyTransceiverIds(
      utility::getCabledPortTranceivers(getHwQsfpEnsemble()));
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  std::map<int32_t, TransceiverInfo> transceiversInfo;
  getHwQsfpEnsemble()->getWedgeManager()->getTransceiversInfo(
      transceiversInfo, std::make_unique<std::vector<int32_t>>(transceivers));

  // Only work with optical transceivers. The offset that this test is
  // writing is not writable on copper/flatMem modules
  auto opticalTransceivers =
      folly::gen::from(transceivers) |
      folly::gen::filter([&transceiversInfo](int32_t tcvrId) {
        auto tcvrInfo = transceiversInfo[tcvrId];
        auto transmitterTech = *tcvrInfo.cable().value_or({}).transmitterTech();
        return transmitterTech == TransmitterTechnology::OPTICAL;
      }) |
      folly::gen::as<std::vector>();

  EXPECT_TRUE(!opticalTransceivers.empty());

  WriteRequest request;
  request.ids() = opticalTransceivers;
  TransceiverIOParameters params;
  // We can write to byte 123 which is password entry page register in both sff
  // and cmis
  // SFF: Password entry area = 123-126
  // CMIS: Password entry area = 122-125
  params.offset() = 123;
  request.parameter() = params;
  request.data() = 0;

  for (auto iteration = 1; iteration <= kI2cStressTestIterations; iteration++) {
    std::unique_ptr<WriteRequest> writeRequest =
        std::make_unique<WriteRequest>(request);
    std::map<int32_t, WriteResponse> currentResponse;
    wedgeManager->writeTransceiverRegister(
        currentResponse, std::move(writeRequest));
    for (auto tcvrId : opticalTransceivers) {
      // Assert that the write operation was successful
      EXPECT_TRUE(currentResponse.find(tcvrId) != currentResponse.end());
      auto curr = currentResponse[tcvrId];
      EXPECT_TRUE(*curr.success());
    }
  }
}

TEST_F(HwTest, cmisPageChange) {
  // Switch between page 0x10 and page 0x11 on CMIS modules on all ports and
  // ensure that page 0x10 reads back the same every time
  auto transceivers = utility::legacyTransceiverIds(
      utility::getCabledPortTranceivers(getHwQsfpEnsemble()));
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  std::map<int32_t, TransceiverInfo> transceiversInfo;
  getHwQsfpEnsemble()->getWedgeManager()->getTransceiversInfo(
      transceiversInfo, std::make_unique<std::vector<int32_t>>(transceivers));

  // Only work with optical CMIS transceivers.
  auto testTransceivers =
      folly::gen::from(transceivers) |
      folly::gen::filter([&transceiversInfo](int32_t tcvrId) {
        const auto& tcvrInfo = transceiversInfo[tcvrId];
        const auto& tcvrState = *tcvrInfo.tcvrState();
        auto transmitterTech =
            *tcvrState.cable().value_or({}).transmitterTech();
        auto mgmtInterface =
            tcvrState.transceiverManagementInterface().value_or({});
        return transmitterTech == TransmitterTechnology::OPTICAL &&
            mgmtInterface == TransceiverManagementInterface::CMIS;
      }) |
      folly::gen::as<std::vector>();

  EXPECT_TRUE(!testTransceivers.empty());

  std::map<int /* page */, std::map<int32_t /* tcvrId */, ReadResponse>>
      previousResponses;
  previousResponses[0x10] = {};

  for (auto iteration = 1; iteration <= kI2cStressTestIterations; iteration++) {
    for (auto page : {0x10, 0x11}) {
      std::unique_ptr<ReadRequest> readRequest =
          std::make_unique<ReadRequest>();
      TransceiverIOParameters params;
      readRequest->ids() = testTransceivers;
      params.offset() = 128; // Start of upper page
      params.page() = page; // Page
      params.length() = 128; // Read Entire page
      readRequest->parameter() = params;

      std::map<int32_t, ReadResponse> currentResponse;
      wedgeManager->readTransceiverRegister(
          currentResponse, std::move(readRequest));

      // Only verify that page 0x10 has not changed. Contents of page 0x11 can
      // change
      if (page != 0x10) {
        continue;
      }
      for (auto tcvrId : testTransceivers) {
        if (previousResponses[page].find(tcvrId) ==
            previousResponses[page].end()) {
          previousResponses[page][tcvrId] = currentResponse[tcvrId];
        }
        for (int i = 0; i < 128; i++) {
          // Expect the previous response and the new response to be the same
          EXPECT_EQ(
              *(previousResponses[page][tcvrId].data()->data() + i),
              *(currentResponse[tcvrId].data()->data() + i));
        }
      }
    }
  }
}
} // namespace facebook::fboss
