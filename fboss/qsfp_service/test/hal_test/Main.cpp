// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <cstdlib>
#include <string>

#include <folly/init/Init.h>
#include <gtest/gtest.h>

namespace {

// Aborts the test binary if any test in the "T0HalTest" suite (firmware
// upgrade) fails, since subsequent tests depend on successful firmware upgrade.
class AbortOnFirmwareUpgradeFailure : public testing::EmptyTestEventListener {
 public:
  void OnTestEnd(const testing::TestInfo& testInfo) override {
    if (std::string(testInfo.test_suite_name()) == "T0HalTest" &&
        testInfo.result()->Failed()) {
      GTEST_LOG_(ERROR)
          << "Firmware upgrade test failed — aborting remaining tests";
      std::exit(EXIT_FAILURE);
    }
  }
};

} // namespace

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  folly::init(&argc, &argv);

  testing::UnitTest::GetInstance()->listeners().Append(
      new AbortOnFirmwareUpgradeFailure());

  return RUN_ALL_TESTS();
}
