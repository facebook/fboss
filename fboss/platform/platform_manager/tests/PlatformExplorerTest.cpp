// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/platform_manager/PlatformExplorer.h"

using namespace ::testing;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

namespace {
void expectVersions(const char* deviceName, const char* versionString) {
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          fmt::format(
              PlatformExplorer::kGroupedFirmwareVersion,
              deviceName,
              versionString)),
      1);
}

void expectHardwareVersion(const char* odsKey, const char* value) {
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(
          fmt::vformat(odsKey, fmt::make_format_args(value))),
      1);
}

PlatformConfig createMinimalPlatformConfig() {
  PlatformConfig platformConfig;
  platformConfig.rootPmUnitName() = "TEST_ROOT_PM";
  platformConfig.rootSlotType() = "TEST_SLOT";
  platformConfig.chassisEepromDevicePath() = "/[CHASSIS_EEPROM]";

  SlotTypeConfig slotTypeConfig;
  slotTypeConfig.pmUnitName() = "TEST_ROOT_PM";
  platformConfig.slotTypeConfigs()["TEST_SLOT"] = slotTypeConfig;

  PmUnitConfig pmUnitConfig;
  platformConfig.pmUnitConfigs()["TEST_ROOT_PM"] = pmUnitConfig;

  return platformConfig;
}
} // namespace

namespace facebook::fboss::platform::platform_manager {

TEST(PlatformExplorerTest, PublishFirmwareVersions) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformFsUtils =
      std::make_shared<PlatformFsUtils>(tmpDir.path().string());

  std::string fpgaFwVerPath = "/run/devmap/inforoms/TEST_FPGA_FWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "1.2", fmt::format("{}/fw_ver", fpgaFwVerPath)));
  std::string cpldFwVerPath = "/run/devmap/cplds/TEST_CPLD_FWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "123.456.789", fmt::format("{}/fw_ver", cpldFwVerPath)));
  std::string cpldBadFwVerPath = "/run/devmap/cplds/TEST_CPLD_BADFWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "123.456.789 // comment", fmt::format("{}/fw_ver", cpldBadFwVerPath)));
  std::string cpldBadFwVer2Path = "/run/devmap/cplds/TEST_CPLD_BADFWVER2";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "123.456.$", fmt::format("{}/fw_ver", cpldBadFwVer2Path)));
  std::string fpgaBadFwVerPath = "/run/devmap/inforoms/TEST_FPGA_BADFWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "1.2.3 a", fmt::format("{}/fw_ver", fpgaBadFwVerPath)));
  std::string fpgaLongFwVerPath = "/run/devmap/inforoms/TEST_FPGA_LONGFWVER";
  std::string bigStr = "0123456789.0123456789.0123456789"; // 32 chars
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      bigStr + "." + bigStr, fmt::format("{}/fw_ver", fpgaLongFwVerPath)));
  // Case with fw_ver under hwmon
  std::string cpldHwmonFwVerPath = "/run/devmap/cplds/FAN0_CPLD_FWVER";
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "1.2.3", fmt::format("{}/hwmon/hwmon20/fw_ver", cpldHwmonFwVerPath)));
  // Case with fw_ver NOT under hwmon, even when hwmon directory exists
  std::string cpldHwmonTrapPath = "/run/devmap/cplds/TAHAN_SMB_CPLD_TRAP";
  EXPECT_TRUE(platformFsUtils->createDirectories(
      fmt::format("{}/hwmon/hwmon20", cpldHwmonTrapPath)));
  EXPECT_TRUE(platformFsUtils->writeStringToFile(
      "7.8.9", fmt::format("{}/fw_ver", cpldHwmonTrapPath)));

  // Non-existent versions
  std::string fpgaNonePath = "/run/devmap/inforoms/NONE";
  EXPECT_TRUE(platformFsUtils->createDirectories(fpgaNonePath));

  PlatformConfig platformConfig;
  platformConfig.symbolicLinkToDevicePath()[fpgaFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldBadFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldBadFwVer2Path] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaBadFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaLongFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldHwmonFwVerPath] = "";
  platformConfig.symbolicLinkToDevicePath()[cpldHwmonTrapPath] = "";
  platformConfig.symbolicLinkToDevicePath()[fpgaNonePath] = "";

  DataStore dataStore(platformConfig);
  facebook::fboss::platform::platform_manager::ScubaLogger scubaLogger(
      *platformConfig.platformName(), dataStore);
  PlatformExplorer explorer(
      platformConfig, dataStore, scubaLogger, platformFsUtils);
  explorer.updateFirmwareVersions();
  explorer.publishFirmwareVersions();

  expectVersions("TEST_FPGA_FWVER", "1.2");
  expectVersions("TEST_CPLD_FWVER", "123.456.789");
  expectVersions(
      "TEST_CPLD_BADFWVER", PlatformExplorer::kFwVerErrorInvalidString);
  expectVersions(
      "TEST_CPLD_BADFWVER2", PlatformExplorer::kFwVerErrorInvalidString);
  expectVersions(
      "TEST_FPGA_BADFWVER", PlatformExplorer::kFwVerErrorInvalidString);
  expectVersions(
      "TEST_FPGA_LONGFWVER", PlatformExplorer::kFwVerErrorInvalidString);
  expectVersions("FAN0_CPLD_FWVER", "1.2.3");
  expectVersions("TAHAN_SMB_CPLD_TRAP", "7.8.9");
  expectVersions("NONE", PlatformExplorer::kFwVerErrorFileNotFound);
}

TEST(PlatformExplorerTest, PublishHardwareVersions) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformFsUtils =
      std::make_shared<PlatformFsUtils>(tmpDir.path().string());

  PlatformConfig platformConfig;
  DataStore dataStore(platformConfig);

  // Directly populate the hardware versions in the dataStore
  // This simulates what updateHardwareVersions() does when reading from EEPROM
  dataStore.updateHardwareVersion(PlatformExplorer::kChassisEepromVersion, "5");
  dataStore.updateHardwareVersion(PlatformExplorer::kProductionState, "GA");
  dataStore.updateHardwareVersion(PlatformExplorer::kProductionSubState, "V3");
  dataStore.updateHardwareVersion(PlatformExplorer::kVariantVersion, "A2");

  facebook::fboss::platform::platform_manager::ScubaLogger scubaLogger(
      *platformConfig.platformName(), dataStore);
  PlatformExplorer explorer(
      platformConfig, dataStore, scubaLogger, platformFsUtils);

  // Test that publishHardwareVersions correctly publishes to ODS
  explorer.publishHardwareVersions();

  // Verify the hardware versions were published to ODS counters
  expectHardwareVersion(PlatformExplorer::kChassisEepromVersionODS, "5");
  expectHardwareVersion(PlatformExplorer::kProductionStateODS, "GA");
  expectHardwareVersion(PlatformExplorer::kProductionSubStateODS, "V3");
  expectHardwareVersion(PlatformExplorer::kVariantVersionODS, "A2");
}

TEST(PlatformExplorerTest, SymlinkExceptionHandling) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformFsUtils =
      std::make_shared<PlatformFsUtils>(tmpDir.path().string());

  auto platformConfig = createMinimalPlatformConfig();
  std::string unsupportedLinkPath = "/run/devmap/unsupported/device";
  platformConfig.symbolicLinkToDevicePath()[unsupportedLinkPath] =
      "/[TEST_DEVICE]";

  DataStore dataStore(platformConfig);
  ScubaLogger scubaLogger(*platformConfig.platformName(), dataStore);
  PlatformExplorer explorer(
      platformConfig, dataStore, scubaLogger, platformFsUtils);

  EXPECT_TRUE(platformFsUtils->createDirectories("/run/devmap/unsupported"));
  EXPECT_NO_THROW(explorer.explore());
  EXPECT_FALSE(platformFsUtils->exists(unsupportedLinkPath));
}

TEST(PlatformExplorerTest, GetFpgaInstanceIdUniquePerDevice) {
  auto platformConfig = createMinimalPlatformConfig();
  auto platformFsUtils = std::make_shared<PlatformFsUtils>();
  DataStore dataStore(platformConfig);
  ScubaLogger scubaLogger(*platformConfig.platformName(), dataStore);

  PlatformExplorer explorer(
      platformConfig, dataStore, scubaLogger, platformFsUtils);

  auto id1 = explorer.getFpgaInstanceId("/", "FPGA1");
  auto id2 = explorer.getFpgaInstanceId("/", "FPGA2");
  auto id3 = explorer.getFpgaInstanceId("/SLOT1", "FPGA1");

  EXPECT_NE(id1, id2);
  EXPECT_NE(id1, id3);
  EXPECT_NE(id2, id3);

  auto id1Again = explorer.getFpgaInstanceId("/", "FPGA1");
  EXPECT_EQ(id1, id1Again);
}

class TestablePlatformExplorer : public PlatformExplorer {
 public:
  using PlatformExplorer::explorationSummary_;
  using PlatformExplorer::PlatformExplorer;
  using PlatformExplorer::updatePmStatus;

  PlatformManagerStatus getLastStatus() const {
    return platformManagerStatus_.copy();
  }
};

TEST(PlatformExplorerTest, ExplorePmUnitWithEmbeddedSensors) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformFsUtils =
      std::make_shared<PlatformFsUtils>(tmpDir.path().string());

  PlatformConfig platformConfig;
  platformConfig.rootPmUnitName() = "TEST_PM_WITH_SENSORS";
  platformConfig.rootSlotType() = "TEST_SLOT";
  platformConfig.chassisEepromDevicePath() = "/[CHASSIS_EEPROM]";
  platformConfig.i2cAdaptersFromCpu() = {};

  SlotTypeConfig slotTypeConfig;
  slotTypeConfig.pmUnitName() = "TEST_PM_WITH_SENSORS";
  platformConfig.slotTypeConfigs()["TEST_SLOT"] = slotTypeConfig;

  PmUnitConfig pmUnitConfig;
  EmbeddedSensorConfig sensorConfig1;
  sensorConfig1.pmUnitScopedName() = "TEMP_SENSOR1";
  sensorConfig1.sysfsPath() = "/sys/class/hwmon/hwmon0/temp1_input";
  EmbeddedSensorConfig sensorConfig2;
  sensorConfig2.pmUnitScopedName() = "TEMP_SENSOR2";
  sensorConfig2.sysfsPath() = "/sys/class/hwmon/hwmon1/temp1_input";
  pmUnitConfig.embeddedSensorConfigs() = {sensorConfig1, sensorConfig2};
  platformConfig.pmUnitConfigs()["TEST_PM_WITH_SENSORS"] = pmUnitConfig;

  DataStore dataStore(platformConfig);
  ScubaLogger scubaLogger(*platformConfig.platformName(), dataStore);
  TestablePlatformExplorer explorer(
      platformConfig, dataStore, scubaLogger, platformFsUtils);

  EXPECT_NO_THROW(explorer.explore());

  // Verify exploration status changed
  auto status = explorer.getLastStatus();
  EXPECT_NE(*status.explorationStatus(), ExplorationStatus::UNSTARTED);

  // Verify embedded sensors were stored in dataStore with correct paths
  std::string sensor1DevicePath = "/[TEMP_SENSOR1]";
  std::string sensor2DevicePath = "/[TEMP_SENSOR2]";
  EXPECT_TRUE(dataStore.hasSysfsPath(sensor1DevicePath));
  EXPECT_TRUE(dataStore.hasSysfsPath(sensor2DevicePath));
  EXPECT_EQ(
      dataStore.getSysfsPath(sensor1DevicePath),
      "/sys/class/hwmon/hwmon0/temp1_input");
  EXPECT_EQ(
      dataStore.getSysfsPath(sensor2DevicePath),
      "/sys/class/hwmon/hwmon1/temp1_input");
}

TEST(PlatformExplorerTest, ExploreSlotWithOutgoingI2cBuses) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformFsUtils =
      std::make_shared<PlatformFsUtils>(tmpDir.path().string());

  PlatformConfig platformConfig;
  platformConfig.rootPmUnitName() = "ROOT_PM";
  platformConfig.rootSlotType() = "ROOT_SLOT";
  platformConfig.chassisEepromDevicePath() = "/[CHASSIS_EEPROM]";
  platformConfig.i2cAdaptersFromCpu() = {};

  SlotTypeConfig rootSlotTypeConfig;
  rootSlotTypeConfig.pmUnitName() = "ROOT_PM";
  platformConfig.slotTypeConfigs()["ROOT_SLOT"] = rootSlotTypeConfig;

  SlotTypeConfig childSlotTypeConfig;
  childSlotTypeConfig.pmUnitName() = "CHILD_PM";
  platformConfig.slotTypeConfigs()["CHILD_SLOT_TYPE"] = childSlotTypeConfig;

  PmUnitConfig rootPmUnitConfig;
  SlotConfig childSlotConfig;
  childSlotConfig.slotType() = "CHILD_SLOT_TYPE";
  childSlotConfig.outgoingI2cBusNames() = {"MUX@0", "MUX@1"};
  rootPmUnitConfig.outgoingSlotConfigs()["CHILD_SLOT@0"] = childSlotConfig;
  platformConfig.pmUnitConfigs()["ROOT_PM"] = rootPmUnitConfig;

  PmUnitConfig childPmUnitConfig;
  platformConfig.pmUnitConfigs()["CHILD_PM"] = childPmUnitConfig;

  DataStore dataStore(platformConfig);
  ScubaLogger scubaLogger(*platformConfig.platformName(), dataStore);
  TestablePlatformExplorer explorer(
      platformConfig, dataStore, scubaLogger, platformFsUtils);

  SlotConfig slotConfig;
  slotConfig.slotType() = "CHILD_SLOT_TYPE";
  slotConfig.outgoingI2cBusNames() = {"MUX@0"};

  EXPECT_THROW(
      explorer.exploreSlot("/", "CHILD_SLOT@0", slotConfig),
      std::runtime_error);
}

TEST(PlatformExplorerTest, GetPmUnitInfoSuccess) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto platformFsUtils =
      std::make_shared<PlatformFsUtils>(tmpDir.path().string());
  auto platformConfig = createMinimalPlatformConfig();
  platformConfig.i2cAdaptersFromCpu() = {};

  DataStore dataStore(platformConfig);
  ScubaLogger scubaLogger(*platformConfig.platformName(), dataStore);
  TestablePlatformExplorer explorer(
      platformConfig, dataStore, scubaLogger, platformFsUtils);

  explorer.explore();
  auto pmUnitInfo = explorer.getPmUnitInfo("/");
  EXPECT_EQ(*pmUnitInfo.name(), "TEST_ROOT_PM");
}

TEST(PlatformExplorerTest, GetPmUnitInfoThrowsForNonExistentSlot) {
  auto platformConfig = createMinimalPlatformConfig();
  auto platformFsUtils = std::make_shared<PlatformFsUtils>();
  DataStore dataStore(platformConfig);
  ScubaLogger scubaLogger(*platformConfig.platformName(), dataStore);

  PlatformExplorer explorer(
      platformConfig, dataStore, scubaLogger, platformFsUtils);

  EXPECT_THROW(
      explorer.getPmUnitInfo("/NON_EXISTENT_SLOT"), std::runtime_error);
}

} // namespace facebook::fboss::platform::platform_manager
