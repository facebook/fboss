// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <chrono>
#include <filesystem>

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/platform_manager/ConfigUtils.h"
#include "fboss/platform/platform_manager/PkgManager.h"
#include "fboss/platform/platform_manager/PlatformManagerHandler.h"

namespace facebook::fboss::platform::platform_manager {
namespace {
const std::map<ExplorationStatus, std::vector<ExplorationStatus>>
    kExplorationStatusTransitions = {
        {ExplorationStatus::UNSTARTED, {ExplorationStatus::IN_PROGRESS}},
        {ExplorationStatus::IN_PROGRESS,
         {ExplorationStatus::SUCCEEDED,
          ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS,
          ExplorationStatus::FAILED}},
        {ExplorationStatus::SUCCEEDED, {}},
        {ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS, {}},
        {ExplorationStatus::FAILED, {}}};
}; // namespace
class CustomPlatformExplorer : public PlatformExplorer {
 public:
  explicit CustomPlatformExplorer(const PlatformConfig& config)
      : PlatformExplorer(config) {
    // Store the initial PlatformManagerStatus defined in PlatformExplorer.
    updatedPmStatuses_.push_back(getPMStatus());
  }
  bool isDeviceExpectedToFail(const std::string& devicePath) {
    return explorationSummary_.isDeviceExpectedToFail(devicePath);
  }
  std::vector<PlatformManagerStatus> updatedPmStatuses_;

 protected:
  void updatePmStatus(const PlatformManagerStatus& newStatus) override {
    PlatformExplorer::updatePmStatus(newStatus);
    updatedPmStatuses_.push_back(newStatus);
  }
};
class PlatformManagerHwTest : public ::testing::Test {
 public:
  void SetUp() override {
    pkgManager_.processAll();
  }
  PlatformManagerStatus getPmStatus() {
    PlatformManagerStatus pmStatus;
    pmClient_->sync_getLastPMStatus(pmStatus);
    return pmStatus;
  }
  std::vector<PlatformManagerStatus> getUpdatedPmStatuses() {
    return platformExplorer_.updatedPmStatuses_;
  }

  PlatformConfig platformConfig_{ConfigUtils().getConfig()};
  CustomPlatformExplorer platformExplorer_{platformConfig_};

 private:
  PkgManager pkgManager_{platformConfig_};
  std::unique_ptr<apache::thrift::Client<PlatformManagerService>> pmClient_{
      apache::thrift::makeTestClient<
          apache::thrift::Client<PlatformManagerService>>(
          std::make_unique<PlatformManagerHandler>(platformExplorer_))};
};

TEST_F(PlatformManagerHwTest, PmExplorationSuccess) {
  auto pmStatus = getPmStatus();
  EXPECT_EQ(*pmStatus.explorationStatus(), ExplorationStatus::UNSTARTED);
  auto now = std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
  platformExplorer_.explore();
  pmStatus = getPmStatus();
  EXPECT_TRUE(
      *pmStatus.explorationStatus() == ExplorationStatus::SUCCEEDED ||
      *pmStatus.explorationStatus() ==
          ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS)
      << fmt::format(
             "Ended with unexpected exploration status {}",
             apache::thrift::util::enumNameSafe(*pmStatus.explorationStatus()));
  EXPECT_GT(*pmStatus.lastExplorationTime(), now);
}

TEST_F(PlatformManagerHwTest, PmExplorationStatusTransitions) {
  platformExplorer_.explore();
  std::optional<ExplorationStatus> currStatus;
  std::vector<ExplorationStatus> nextStatuses = {ExplorationStatus::UNSTARTED};
  auto updatedPmStatuses = getUpdatedPmStatuses();
  EXPECT_FALSE(updatedPmStatuses.empty());
  for (const auto& nextPmStatus : updatedPmStatuses) {
    auto nextExplorationStatus = *nextPmStatus.explorationStatus();
    EXPECT_NE(
        std::find(
            nextStatuses.begin(), nextStatuses.end(), nextExplorationStatus),
        nextStatuses.end())
        << fmt::format(
               "Detected invalid ExplorationStatus transition from {} to {}",
               currStatus ? apache::thrift::util::enumNameSafe(*currStatus)
                          : "<NULL>",
               apache::thrift::util::enumNameSafe(nextExplorationStatus));
    EXPECT_NO_THROW(
        nextStatuses = kExplorationStatusTransitions.at(nextExplorationStatus))
        << fmt::format(
               "Undefined next ExplorationStatus mapping in kExplorationStatusTransitions for {}",
               apache::thrift::util::enumNameSafe(nextExplorationStatus));
    currStatus = nextExplorationStatus;
  }
  EXPECT_TRUE(currStatus.has_value());
  EXPECT_TRUE(
      *currStatus == ExplorationStatus::SUCCEEDED ||
      *currStatus == ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS)
      << fmt::format(
             "Ended with unexpected exploration status {}",
             apache::thrift::util::enumNameSafe(*currStatus));
}

TEST_F(PlatformManagerHwTest, Symlinks) {
  std::filesystem::remove_all("/run/devmap");
  EXPECT_FALSE(std::filesystem::exists("/run/devmap"));
  platformExplorer_.explore();
  for (const auto& [symlink, devicePath] :
       *platformConfig_.symbolicLinkToDevicePath()) {
    // Skip unsupported device in this hardware.
    if (platformExplorer_.isDeviceExpectedToFail(devicePath)) {
      continue;
    }
    EXPECT_TRUE(std::filesystem::exists(symlink))
        << fmt::format("{} doesn't exist", symlink);
    EXPECT_TRUE(std::filesystem::is_symlink(symlink))
        << fmt::format("{} isn't a symlink", symlink);
  }
}

} // namespace facebook::fboss::platform::platform_manager

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
