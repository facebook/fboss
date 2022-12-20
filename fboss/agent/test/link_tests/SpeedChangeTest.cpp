// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonFileUtils.h"

using namespace ::testing;
using namespace facebook::fboss;

class SpeedChangeTest : public LinkTest {
 public:
  void TearDown() override;
  void setupConfigFlag() override;

 private:
  std::string originalConfigCopy;
};

void SpeedChangeTest::setupConfigFlag() {
  // Coldboot iteration -
  //    SetUp -
  //      1. Make a copy of the original config. Existing copy is overridden
  //      2. The speeds and profiles are modified by this test in the original
  //      config.
  //    TearDown - Do nothing. Keep the modified config in the FLAGS_config
  //    path.
  // Warmboot iteration -
  //    SetUp - Do nothing. Agent loads up with the modified config (setup by
  //      coldboot iteration)
  //    TearDown - Copy is deleted if we are not asked to setup_for_warmboot
  //    again
  auto linkTestDir = FLAGS_volatile_state_dir + "/link_test";
  originalConfigCopy =
      linkTestDir + "/agent_speed_change_test_original_config.conf";

  // Create a copy of the original config if we haven't created one before.
  if (!boost::filesystem::exists(originalConfigCopy)) {
    // Likely a coldboot iteration
    utilCreateDir(linkTestDir);
    boost::filesystem::copy_file(FLAGS_config, originalConfigCopy);
  }
  // Make sure the copy exists
  CHECK(boost::filesystem::exists(originalConfigCopy));
}

void SpeedChangeTest::TearDown() {
  if (!FLAGS_setup_for_warmboot) {
    // If this test wasn't setup for warmboot, revert back the original config
    // in FLAGS_config path. Also clean up the copy
    boost::filesystem::copy_file(
        originalConfigCopy,
        FLAGS_config,
        boost::filesystem::copy_option::overwrite_if_exists);
    CHECK(removeFile(originalConfigCopy));
  }
  LinkTest::TearDown();
}
