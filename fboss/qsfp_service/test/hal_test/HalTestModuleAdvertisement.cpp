// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/test/hal_test/HalTest.h"

#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {

// Verify that every present transceiver reports a known (non-UNKNOWN) media
// interface code. A module returning UNKNOWN likely has unreadable or
// unsupported application advertising fields.
TEST_F(T1HalTest, verifyModuleMediaInterfaceIsNotUnknown) {
  for (auto tcvrId : getPresentTransceiverIds()) {
    auto* module = getModule(tcvrId);

    module->updateQsfpData();
    auto mediaInterface = module->getModuleMediaInterface();
    // Log the media interface code for each transceiver
    XLOG(INFO) << "Transceiver " << tcvrId << " media interface code: "
               << apache::thrift::util::enumNameSafe(mediaInterface);
    EXPECT_NE(mediaInterface, MediaInterfaceCode::UNKNOWN)
        << "Transceiver " << tcvrId << " reported UNKNOWN media interface code";
  }
}

// Verify that the datapath init max delay advertised by each present module
// is within expected bounds: at most 5 seconds for standard modules, and
// at most 1 minute for 800G ZR modules.
TEST_F(T1HalTest, verifyDatapathMaxDelayFromModuleSpec) {
  constexpr uint64_t kMaxDelayUsecDefault = 5'000'000; // 5 seconds
  constexpr uint64_t kMaxDelayUsecZR = 60'000'000; // 1 minute

  for (auto tcvrId : getPresentTransceiverIds()) {
    auto* module = getModule(tcvrId);

    module->updateQsfpData();

    auto* cmisModule = dynamic_cast<CmisModule*>(module);
    ASSERT_NE(cmisModule, nullptr)
        << "Transceiver " << tcvrId << " is not CMIS";

    auto maxDelay =
        cmisModule->getDatapathMaxDelayFromModuleSpec(/*init=*/true);
    ASSERT_TRUE(maxDelay.has_value())
        << "Transceiver " << tcvrId
        << " failed to read datapath max delay from module spec";

    auto mediaInterface = module->getModuleMediaInterface();
    uint64_t expectedMaxDelay = (mediaInterface == MediaInterfaceCode::ZR_800G)
        ? kMaxDelayUsecZR
        : kMaxDelayUsecDefault;

    EXPECT_LE(*maxDelay, expectedMaxDelay)
        << "Transceiver " << tcvrId << " datapath init delay " << *maxDelay
        << " usec exceeds expected max " << expectedMaxDelay << " usec"
        << " (media interface: "
        << apache::thrift::util::enumNameSafe(mediaInterface) << ")";
  }
}

} // namespace facebook::fboss
