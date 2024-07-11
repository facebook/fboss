// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/HwTest.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverTest.h"

using namespace ::testing;
using namespace facebook::fboss;

class HwTransceiverConfigValidationTest : public HwTransceiverTest {
 public:
  // Mark the port up to trigger module configuration
  HwTransceiverConfigValidationTest() : HwTransceiverTest(true) {}

 protected:
  void SetUp() override {
    FLAGS_enable_tcvr_validation = true;
    HwTransceiverTest::SetUp();
  }
};

TEST_F(HwTransceiverConfigValidationTest, validateAllActiveTransceivers) {
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  std::vector<int32_t> invalidTransceivers, validTransceivers;
  auto all_ids = getExpectedLegacyTransceiverIds();
  for (auto id : *all_ids) {
    std::string notValidatedReason;
    TransceiverID tcvrID = TransceiverID(id);
    if (!wedgeManager->isValidTransceiver(tcvrID) ||
        !wedgeManager->getTransceiverInfo(tcvrID).tcvrState()->get_present()) {
      continue;
    }
    if (wedgeManager->validateTransceiverById(
            tcvrID, notValidatedReason, false)) {
      validTransceivers.push_back(id);
    } else {
      invalidTransceivers.push_back(id);
    }
  }

  if (!invalidTransceivers.empty()) {
    XLOG(INFO) << "Transceivers [" << folly::join(",", invalidTransceivers)
               << "] have non-validated configs.";
  }
  if (!validTransceivers.empty()) {
    XLOG(INFO) << "Transceivers [" << folly::join(",", validTransceivers)
               << "] all have validated configurations.";
  }

  EXPECT_TRUE(invalidTransceivers.empty());
}
