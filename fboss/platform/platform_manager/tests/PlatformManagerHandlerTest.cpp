// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>

#include "fboss/platform/platform_manager/PlatformManagerHandler.h"

using namespace ::testing;
using namespace facebook::fboss::platform::platform_manager;

namespace facebook::fboss::platform::platform_manager {

namespace {
std::vector<uint8_t> createTestEepromV5Data() {
  // clang-format off
  return {
      0xFB, 0xFB, 0x05, 0xFF,  // Header: Magic, Version 5, Reserved
      0x01, 0x0D, 'T', 'E', 'S', 'T', '_', 'P', 'R', 'O', 'D', 'U', 'C', 'T', 0x00,  // Product name
      0x02, 0x08, '1', '2', '3', '4', '5', '6', '7', '8',  // Product serial
      0x03, 0x08, 'S', 'Y', 'S', 'A', '1', '2', '3', '4',  // System assembly part number
      0x04, 0x0C, 'P', 'C', 'B', 'A', '0', '0', '0', '0', '0', '0', '0', '1',  // PCBA part number
      0x05, 0x08, 'P', 'C', 'B', '1', '2', '3', '4', '5',  // PCB part number
      0x06, 0x02, 0x00, 0x01,  // ODM PCBA part number
      0x07, 0x0B, 'M', 'F', 'G', '_', '2', '0', '2', '4', '0', '1', '1',  // Manufacturing date
      0x08, 0x02, 0x00, 0x02,  // Manufacturing location
      0x09, 0x02, 0x00, 0x03,  // FB PCBA part number
      0x0A, 0x02, 0x00, 0x04,  // FB PCB part number
      0x0B, 0x0C, 'S', 'E', 'R', 'I', 'A', 'L', '1', '2', '3', '4', '5', '6',  // Extended MAC base
      0x0C, 0x0C, 'A', 'S', 'S', 'E', 'T', '1', '2', '3', '4', '5', '6', '7',  // Extended MAC address size
      0x0D, 0x08, 'L', 'O', 'C', 'A', 'L', 'M', 'A', 'C',  // Location on fabric
      0x0E, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,  // MAC address field 1
      0x0F, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,  // MAC address field 2
      0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,  // MAC address field 3
      0x11, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,  // MAC address field 4
      0x12, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,  // MAC address field 5
      0x13, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,  // MAC address field 6
      0x14, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,  // MAC address field 7
      0xFA, 0x02, 0x00, 0x00,  // CRC field
      0xFF  // End marker
  };
  // clang-format on
}

std::vector<uint8_t> createTestEepromV6Data() {
  // clang-format off
  return {
      0xFB, 0xFB, 0x06, 0xFF,  // Header: Magic, Version 6, Reserved
      0x01, 0x0D, 'F', 'I', 'R', 'S', 'T', '_', 'S', 'Q', 'U', 'E', 'E', 'Z', 'E',  // Product name
      0x02, 0x08, '2', '0', '1', '2', '3', '4', '5', '6',  // Product part number
      0x03, 0x08, 'S', 'Y', 'S', 'A', '1', '2', '3', '4',  // System assembly part number
      0x04, 0x0C, 'P', 'C', 'B', 'A', '1', '2', '3', '4', '5', '6', '7', ' ',  // Meta PCBA part number
      0x05, 0x0C, 'P', 'C', 'B', '1', '2', '3', '4', '5', '6', '7', '8', ' ',  // Meta PCB part number
      0x06, 0x0C, 'M', 'Y', 'O', 'D', 'M', '1', '2', '3', '4', '5', '6', '7',  // ODM/JDM PCBA part number
      0x07, 0x0D, 'O', 'S', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',  // ODM/JDM PCBA serial
      0x08, 0x01, 0x01,  // Production state
      0x09, 0x01, 0x00,  // Production sub-state
      0x0A, 0x01, 0x01,  // Re-Spin/Variant indicator
      0x0B, 0x0D, 'P', 'S', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'A',  // Product serial
      0x0C, 0x07, 'U', 'N', 'A', '_', 'M', 'A', 'S',  // System manufacturer
      0x0D, 0x08, '2', '0', '1', '3', '0', '2', '0', '3',  // System manufacturing date
      0x0E, 0x05, 'T', 'E', 'R', 'Z', 'O',  // PCB manufacturer
      0x0F, 0x09, 'J', 'U', 'I', 'C', 'E', 'T', 'O', 'R', 'Y',  // Assembled at
      0x10, 0x07, 'B', 'U', 'D', 'O', 'K', 'A', 'N',  // EEPROM location on fabric
      0x11, 0x08, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x01, 0x02,  // X86 CPU MAC
      0x12, 0x08, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0x03, 0x04,  // BMC MAC
      0x13, 0x08, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x02, 0x00,  // Switch ASIC MAC
      0x14, 0x08, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x00, 0x02,  // META Reserved MAC
      0x15, 0x01, 0x01,  // RMA (V6 only)
      0x65, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,  // Vendor Defined Field 1
      0x66, 0x05, 'H', 'e', 'l', 'l', 'o',  // Vendor Defined Field 2
      0xFA, 0x02, 0x4A, 0x05,  // CRC16
      0xFF  // End marker
  };
  // clang-format on
}

} // namespace

class MockPlatformExplorer : public PlatformExplorer {
 public:
  explicit MockPlatformExplorer(
      const PlatformConfig& config,
      DataStore& dataStore,
      ScubaLogger& scubaLogger)
      : PlatformExplorer(
            config,
            dataStore,
            scubaLogger,
            std::make_shared<PlatformFsUtils>()) {}

  void setPMStatus(const PlatformManagerStatus& status) {
    updatePmStatus(status);
  }
};

class PlatformManagerHandlerTest : public Test {
 protected:
  void SetUp() override {
    setupDefaultConfig();
    initializeComponents();
  }

  void setupDefaultConfig() {
    config_.platformName() = "TestPlatform";
    config_.bspKmodsRpmName() = "test-bsp-rpm";
    config_.bspKmodsRpmVersion() = "1.0.0";
    config_.chassisEepromDevicePath() = "/chassis_eeprom";
  }

  void initializeComponents() {
    dataStore_ = std::make_unique<DataStore>(config_);
    scubaLogger_ =
        std::make_unique<ScubaLogger>(*config_.platformName(), *dataStore_);
    explorer_ = std::make_unique<MockPlatformExplorer>(
        config_, *dataStore_, *scubaLogger_);
    handler_ = std::make_unique<PlatformManagerHandler>(
        *explorer_, *dataStore_, config_);
  }

  void setExplorationStatus(ExplorationStatus status) {
    PlatformManagerStatus pmStatus;
    pmStatus.explorationStatus() = status;
    explorer_->setPMStatus(pmStatus);
  }

  PlatformConfig config_;
  std::unique_ptr<DataStore> dataStore_;
  std::unique_ptr<ScubaLogger> scubaLogger_;
  std::unique_ptr<MockPlatformExplorer> explorer_;
  std::unique_ptr<PlatformManagerHandler> handler_;
};

class GetPmUnitInfoExplorationStatusTest
    : public PlatformManagerHandlerTest,
      public WithParamInterface<ExplorationStatus> {};

TEST_P(GetPmUnitInfoExplorationStatusTest, ThrowsWhenExplorationNotSucceeded) {
  setExplorationStatus(GetParam());

  auto request = std::make_unique<PmUnitInfoRequest>();
  request->slotPath() = "/";

  PmUnitInfoResponse response;
  try {
    handler_->getPmUnitInfo(response, std::move(request));
    FAIL() << "Expected PlatformManagerError to be thrown";
  } catch (const PlatformManagerError& e) {
    EXPECT_EQ(
        *e.errorCode(), PlatformManagerErrorCode::EXPLORATION_NOT_SUCCEEDED);
  }
}

INSTANTIATE_TEST_SUITE_P(
    ExplorationStatuses,
    GetPmUnitInfoExplorationStatusTest,
    Values(ExplorationStatus::FAILED, ExplorationStatus::IN_PROGRESS));

TEST_F(PlatformManagerHandlerTest, GetPmUnitInfo_ThrowsOnInvalidSlotPath) {
  setExplorationStatus(ExplorationStatus::SUCCEEDED);

  auto request = std::make_unique<PmUnitInfoRequest>();
  request->slotPath() = "/NONEXISTENT_SLOT@0";

  PmUnitInfoResponse response;
  try {
    handler_->getPmUnitInfo(response, std::move(request));
    FAIL() << "Expected PlatformManagerError to be thrown";
  } catch (const PlatformManagerError& e) {
    EXPECT_EQ(*e.errorCode(), PlatformManagerErrorCode::PM_UNIT_NOT_FOUND);
  }
}

TEST_F(PlatformManagerHandlerTest, GetPmUnitInfo_ReturnsValidInfoOnSuccess) {
  setExplorationStatus(ExplorationStatus::SUCCEEDED);

  // Setup: Populate DataStore with a valid PmUnit
  PmUnitVersion version;
  version.productProductionState() = 2;
  version.productVersion() = 13;
  version.productSubVersion() = 1;

  dataStore_->updatePmUnitName("/TEST_SLOT@0", "TEST_PM_UNIT");
  dataStore_->updatePmUnitVersion("/TEST_SLOT@0", version);

  auto request = std::make_unique<PmUnitInfoRequest>();
  request->slotPath() = "/TEST_SLOT@0";

  PmUnitInfoResponse response;
  handler_->getPmUnitInfo(response, std::move(request));

  EXPECT_EQ(*response.pmUnitInfo()->name(), "TEST_PM_UNIT");
  EXPECT_EQ(*response.pmUnitInfo()->version()->productProductionState(), 2);
  EXPECT_EQ(*response.pmUnitInfo()->version()->productVersion(), 13);
  EXPECT_EQ(*response.pmUnitInfo()->version()->productSubVersion(), 1);
}

TEST_F(PlatformManagerHandlerTest, GetAllPmUnits_ReturnsPopulatedMapWithUnits) {
  PmUnitVersion version1;
  version1.productProductionState() = 1;
  version1.productVersion() = 10;
  version1.productSubVersion() = 0;

  PmUnitVersion version2;
  version2.productProductionState() = 2;
  version2.productVersion() = 20;
  version2.productSubVersion() = 5;

  DataStore dataStore(config_);
  dataStore.updatePmUnitName("/SLOT_A@0", "PM_UNIT_A");
  dataStore.updatePmUnitVersion("/SLOT_A@0", version1);
  dataStore.updatePmUnitName("/SLOT_B@1", "PM_UNIT_B");
  dataStore.updatePmUnitVersion("/SLOT_B@1", version2);

  ScubaLogger scubaLogger(*config_.platformName(), dataStore);
  MockPlatformExplorer explorer(config_, dataStore, scubaLogger);
  PlatformManagerHandler handler(explorer, dataStore, config_);

  PmUnitsResponse response;
  handler.getAllPmUnits(response);

  EXPECT_EQ(response.pmUnits()->size(), 2);

  auto& unitA = response.pmUnits()->at("/SLOT_A@0");
  EXPECT_EQ(*unitA.name(), "PM_UNIT_A");
  EXPECT_EQ(*unitA.version()->productProductionState(), 1);
  EXPECT_EQ(*unitA.version()->productVersion(), 10);

  auto& unitB = response.pmUnits()->at("/SLOT_B@1");
  EXPECT_EQ(*unitB.name(), "PM_UNIT_B");
  EXPECT_EQ(*unitB.version()->productProductionState(), 2);
  EXPECT_EQ(*unitB.version()->productVersion(), 20);
}

TEST_F(
    PlatformManagerHandlerTest,
    GetEepromContents_ThrowsWhenChassisEepromNotFound) {
  auto request = std::make_unique<PmUnitInfoRequest>();
  request->slotPath() = "";

  EepromContentResponse response;
  try {
    handler_->getEepromContents(response, std::move(request));
    FAIL() << "Expected PlatformManagerError to be thrown";
  } catch (const PlatformManagerError& e) {
    EXPECT_EQ(
        *e.errorCode(), PlatformManagerErrorCode::EEPROM_CONTENTS_NOT_FOUND);
  }
}

TEST_F(
    PlatformManagerHandlerTest,
    GetEepromContents_ThrowsOnNonexistentSlotPath) {
  auto request = std::make_unique<PmUnitInfoRequest>();
  request->slotPath() = "/NONEXISTENT_SLOT";

  EepromContentResponse response;
  try {
    handler_->getEepromContents(response, std::move(request));
    FAIL() << "Expected PlatformManagerError to be thrown";
  } catch (const PlatformManagerError& e) {
    EXPECT_EQ(
        *e.errorCode(), PlatformManagerErrorCode::EEPROM_CONTENTS_NOT_FOUND);
  }
}

TEST_F(
    PlatformManagerHandlerTest,
    GetEepromContents_ReturnsValidEepromV5Contents) {
  auto tmpDir = folly::test::TemporaryDirectory();
  std::string eepromPath = tmpDir.path().string() + "/eeprom";

  std::vector<uint8_t> eepromData = createTestEepromV5Data();
  folly::writeFile(eepromData, eepromPath.c_str());

  DataStore dataStore(config_);
  FbossEepromInterface eepromInterface(eepromPath, 0);
  dataStore.updateEepromContents("/test_slot", eepromInterface);

  ScubaLogger scubaLogger(*config_.platformName(), dataStore);
  MockPlatformExplorer explorer(config_, dataStore, scubaLogger);
  PlatformManagerHandler handler(explorer, dataStore, config_);

  auto request = std::make_unique<PmUnitInfoRequest>();
  request->slotPath() = "/test_slot";

  EepromContentResponse response;
  handler.getEepromContents(response, std::move(request));

  EXPECT_EQ(*response.eepromContents()->version(), 5);
  EXPECT_EQ(*response.eepromContents()->productName(), "TEST_PRODUCT");
}

TEST_F(
    PlatformManagerHandlerTest,
    GetEepromContents_ReturnsValidEepromV6Contents) {
  auto tmpDir = folly::test::TemporaryDirectory();
  std::string eepromPath = tmpDir.path().string() + "/eeprom";

  std::vector<uint8_t> eepromData = createTestEepromV6Data();
  folly::writeFile(eepromData, eepromPath.c_str());

  DataStore dataStore(config_);
  FbossEepromInterface eepromInterface(eepromPath, 0);
  dataStore.updateEepromContents("/test_slot_v6", eepromInterface);

  ScubaLogger scubaLogger(*config_.platformName(), dataStore);
  MockPlatformExplorer explorer(config_, dataStore, scubaLogger);
  PlatformManagerHandler handler(explorer, dataStore, config_);

  auto request = std::make_unique<PmUnitInfoRequest>();
  request->slotPath() = "/test_slot_v6";

  EepromContentResponse response;
  handler.getEepromContents(response, std::move(request));

  EXPECT_EQ(*response.eepromContents()->version(), 6);
  EXPECT_EQ(*response.eepromContents()->productName(), "FIRST_SQUEEZE");
  EXPECT_EQ(*response.eepromContents()->productPartNumber(), "20123456");
  EXPECT_EQ(*response.eepromContents()->rma(), "1");
  EXPECT_EQ(*response.eepromContents()->vendorDefinedField1(), "0x0101010101");
  EXPECT_EQ(*response.eepromContents()->vendorDefinedField2(), "0x48656c6c6f");
}

} // namespace facebook::fboss::platform::platform_manager
