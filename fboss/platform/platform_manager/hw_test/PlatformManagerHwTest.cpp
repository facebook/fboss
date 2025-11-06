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

// Find the number of LEDs configured for a given xcvr number.
int getNumLedsForXcvr(int xcvrNum, const PlatformConfig& platformConfig) {
  int ledCount = 0;

  for (const auto& [pmUnitName, pmUnitConfig] :
       *platformConfig.pmUnitConfigs()) {
    for (const auto& pciDeviceConfig : *pmUnitConfig.pciDeviceConfigs()) {
      // Check individual LED control configs
      for (const auto& ledCtrlConfig : *pciDeviceConfig.ledCtrlConfigs()) {
        if (*ledCtrlConfig.portNumber() == xcvrNum) {
          ledCount++;
        }
      }

      // Check LED control block configs
      for (const auto& ledBlockConfig :
           *pciDeviceConfig.ledCtrlBlockConfigs()) {
        int startPort = *ledBlockConfig.startPort();
        int numPorts = *ledBlockConfig.numPorts();
        int ledPerPort = *ledBlockConfig.ledPerPort();

        // Check if xcvrNum falls within this block's port range
        if (xcvrNum >= startPort && xcvrNum < startPort + numPorts) {
          ledCount += ledPerPort;
        }
      }
    }
  }

  return ledCount;
}
}; // namespace

namespace fs = std::filesystem;

class PlatformExplorerWrapper : public PlatformExplorer {
 public:
  explicit PlatformExplorerWrapper(const PlatformConfig& config)
      : PlatformExplorer(config) {
    // Store the initial PlatformManagerStatus defined in PlatformExplorer.
    updatedPmStatuses_.push_back(getPMStatus());
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
  PlatformManagerStatus getPmStatus() {
    PlatformManagerStatus pmStatus;
    pmClient_->sync_getLastPMStatus(pmStatus);
    return pmStatus;
  }
  std::vector<PlatformManagerStatus> getUpdatedPmStatuses() {
    return platformExplorer_.updatedPmStatuses_;
  }
  void explorationOk() {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();
    pkgManager_.processAll();
    platformExplorer_.explore();
    auto pmStatus = getPmStatus();
    EXPECT_TRUE(
        *pmStatus.explorationStatus() == ExplorationStatus::SUCCEEDED ||
        *pmStatus.explorationStatus() ==
            ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS)
        << fmt::format(
               "Ended with unexpected exploration status {}",
               apache::thrift::util::enumNameSafe(
                   *pmStatus.explorationStatus()));
    EXPECT_GT(*pmStatus.lastExplorationTime(), now);
  }

  PlatformConfig platformConfig_{ConfigUtils().getConfig()};
  PlatformExplorerWrapper platformExplorer_{platformConfig_};
  PkgManager pkgManager_{platformConfig_};
  std::optional<DataStore> ds =
      platformExplorer_.getDataStore().value_or(DataStore(platformConfig_));
  std::unique_ptr<apache::thrift::Client<PlatformManagerService>> pmClient_{
      apache::thrift::makeTestClient<
          apache::thrift::Client<PlatformManagerService>>(
          std::make_unique<PlatformManagerHandler>(
              platformExplorer_,
              ds.value(),
              platformConfig_))};
};

TEST_F(PlatformManagerHwTest, ExploreAsDeployed) {
  explorationOk();
}

TEST_F(PlatformManagerHwTest, ExploreAfterUninstallingKmods) {
  pkgManager_.removeInstalledRpms();
  explorationOk();
}

TEST_F(PlatformManagerHwTest, ExploreAfterUnloadingKmods) {
  pkgManager_.unloadBspKmods();
  explorationOk();
}

TEST_F(PlatformManagerHwTest, PmExplorationStatusTransitions) {
  explorationOk();
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
  fs::remove_all("/run/devmap");
  EXPECT_FALSE(fs::exists("/run/devmap"));
  explorationOk();
  for (const auto& [symlink, devicePath] :
       *platformConfig_.symbolicLinkToDevicePath()) {
    EXPECT_TRUE(fs::exists(symlink))
        << fmt::format("{} doesn't exist", symlink);
    EXPECT_TRUE(fs::is_symlink(symlink))
        << fmt::format("{} isn't a symlink", symlink);
  }
}

// Verifies the control files expected by qsfp_service are properly created.
TEST_F(PlatformManagerHwTest, XcvrCtrlFiles) {
  fs::remove_all("/run/devmap/xcvrs");
  EXPECT_FALSE(fs::exists("/run/devmap/xcvrs"));
  explorationOk();
  for (auto xcvrId = 1; xcvrId <= *platformConfig_.numXcvrs(); xcvrId++) {
    auto xcvrCtrlDir =
        fs::path(fmt::format("/run/devmap/xcvrs/xcvr_ctrl_{}", xcvrId));
    fs::path presenceFile, resetFile;
    std::string presenceFileName, resetFileName;
    if (*platformConfig_.bspKmodsRpmName() == "cisco_bsp_kmods") {
      presenceFileName = "xcvr_present";
      resetFileName = "xcvr_reset";
    } else {
      presenceFileName = fmt::format("xcvr_present_{}", xcvrId);
      resetFileName = fmt::format("xcvr_reset_{}", xcvrId);
    }
    presenceFile = xcvrCtrlDir / fs::path(presenceFileName);
    resetFile = xcvrCtrlDir / fs::path(resetFileName);
    EXPECT_TRUE(fs::exists(presenceFile))
        << fmt::format("{} doesn't exist", presenceFile.string());
    EXPECT_TRUE(fs::exists(resetFile))
        << fmt::format("{} doesn't exist", resetFile.string());
    if (fs::exists(presenceFile)) {
      auto presenceFileContent =
          PlatformFsUtils().getStringFileContent(presenceFile);
      ASSERT_TRUE(presenceFileContent);
      int presenceValue{-1};
      ASSERT_NO_THROW(
          presenceValue = std::stoi(*presenceFileContent, nullptr, 0));
      EXPECT_TRUE(presenceValue == 0 || presenceValue == 1);
    }
  }
}

TEST_F(PlatformManagerHwTest, XcvrIoFiles) {
  fs::remove_all("/run/devmap/xcvrs");
  EXPECT_FALSE(fs::exists("/run/devmap/xcvrs"));
  explorationOk();
  for (auto xcvrId = 1; xcvrId <= *platformConfig_.numXcvrs(); xcvrId++) {
    auto xcvrIoPath =
        fs::path(fmt::format("/run/devmap/xcvrs/xcvr_io_{}", xcvrId));
    EXPECT_TRUE(fs::is_character_file(xcvrIoPath))
        << fmt::format("{} isn't a character file", xcvrIoPath.string());
  }
}

TEST_F(PlatformManagerHwTest, XcvrLedFiles) {
  fs::remove_all("/run/devmap/xcvrs");
  EXPECT_FALSE(fs::exists("/run/devmap/xcvrs"));
  explorationOk();
  for (auto xcvrNum = 1; xcvrNum <= *platformConfig_.numXcvrs(); xcvrNum++) {
    auto numLeds = getNumLedsForXcvr(xcvrNum, platformConfig_);
    for (auto ledNum = 1; ledNum <= numLeds; ledNum++) {
      auto blueLedDir = fs::path(
          fmt::format(
              "/sys/class/leds/port{}_led{}:blue:status", xcvrNum, ledNum));
      auto amberLedDir = fs::path(
          fmt::format(
              "/sys/class/leds/port{}_led{}:amber:status", xcvrNum, ledNum));
      XLOG(DBG2) << fmt::format(
          "Checking {} and {}", blueLedDir.string(), amberLedDir.string());
      for (auto& ledFile : {"brightness", "max_brightness", "trigger"}) {
        auto blueLedFullPath = blueLedDir / fs::path(ledFile);
        auto amberLedFullPath = amberLedDir / fs::path(ledFile);
        EXPECT_TRUE(fs::exists(blueLedFullPath))
            << fmt::format("{} doesn't exist", blueLedFullPath.string());
        EXPECT_TRUE(fs::exists(amberLedFullPath))
            << fmt::format("{} doesn't exist", amberLedFullPath.string());
      }
    }
  }
}

} // namespace facebook::fboss::platform::platform_manager

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
