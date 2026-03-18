// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/core.h>

#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/test/hal_test/HalTest.h"

#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {

// Verify that every present transceiver reports a known (non-UNKNOWN) media
// interface code. A module returning UNKNOWN likely has unreadable or
// unsupported application advertising fields.
TEST_F(T1HalTest, verifyModuleMediaInterfaceIsNotUnknown) {
  forEachTransceiverParallel(
      [this](hal_test::TransceiverTestResult& result, int tcvrId) {
        auto* module = getModule(tcvrId);

        module->updateQsfpData();
        auto mediaInterface = module->getModuleMediaInterface();
        XLOG(INFO) << "Transceiver " << tcvrId << " media interface code: "
                   << apache::thrift::util::enumNameSafe(mediaInterface);
        HAL_CHECK(
            result,
            mediaInterface != MediaInterfaceCode::UNKNOWN,
            fmt::format(
                "Transceiver {} reported UNKNOWN media interface code",
                tcvrId));
      });
}

// Verify that the datapath init max delay advertised by each present module
// is within expected bounds: at most 5 seconds for standard modules, and
// at most 1 minute for 800G ZR modules.
TEST_F(T1HalTest, verifyDatapathMaxDelayFromModuleSpec) {
  constexpr uint64_t kMaxDelayUsecDefault = 5'000'000; // 5 seconds
  constexpr uint64_t kMaxDelayUsecZR = 60'000'000; // 1 minute

  forEachTransceiverParallel([this, kMaxDelayUsecDefault, kMaxDelayUsecZR](
                                 hal_test::TransceiverTestResult& result,
                                 int tcvrId) {
    auto* module = getModule(tcvrId);

    module->updateQsfpData();

    auto* cmisModule = dynamic_cast<CmisModule*>(module);
    HAL_CHECK_FATAL_VOID(
        result,
        cmisModule != nullptr,
        fmt::format("Transceiver {} is not CMIS", tcvrId));

    auto maxDelay =
        cmisModule->getDatapathMaxDelayFromModuleSpec(/*init=*/true);
    HAL_CHECK_FATAL_VOID(
        result,
        maxDelay.has_value(),
        fmt::format(
            "Transceiver {} failed to read datapath max delay from module spec",
            tcvrId));

    auto mediaInterface = module->getModuleMediaInterface();
    uint64_t expectedMaxDelay = (mediaInterface == MediaInterfaceCode::ZR_800G)
        ? kMaxDelayUsecZR
        : kMaxDelayUsecDefault;

    HAL_CHECK(
        result,
        *maxDelay <= expectedMaxDelay,
        fmt::format(
            "Transceiver {} datapath init delay {} usec exceeds expected max {} usec (media interface: {})",
            tcvrId,
            *maxDelay,
            expectedMaxDelay,
            apache::thrift::util::enumNameSafe(mediaInterface)));
  });
}

} // namespace facebook::fboss
