/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 */

#include "fboss/platform/platform_checks/checks/i801SmbusTimeoutCheck.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/helpers/MockPlatformFsUtils.h"
#include "fboss/platform/helpers/MockPlatformUtils.h"

using namespace ::testing;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_checks;

namespace {

class Mocki801SmbusTimeoutCheck : public i801SmbusTimeoutCheck {
 public:
  explicit Mocki801SmbusTimeoutCheck(
      std::shared_ptr<MockPlatformFsUtils> platformFsUtils,
      std::shared_ptr<MockPlatformUtils> platformUtils)
      : i801SmbusTimeoutCheck(platformFsUtils, platformUtils) {}

  MOCK_METHOD(
      std::unique_ptr<FbossEepromInterface>,
      createEepromInterface,
      (const std::string& path, uint16_t offset),
      (override));
};

} // namespace

class i801SmbusTimeoutCheckTest : public ::testing::Test {
 protected:
  void SetUp() override {
    platformFsUtils_ = std::make_shared<MockPlatformFsUtils>();
    platformUtils_ = std::make_shared<MockPlatformUtils>();
    check_ = std::make_unique<Mocki801SmbusTimeoutCheck>(
        platformFsUtils_, platformUtils_);
  }

  std::shared_ptr<MockPlatformFsUtils> platformFsUtils_;
  std::shared_ptr<MockPlatformUtils> platformUtils_;
  std::unique_ptr<Mocki801SmbusTimeoutCheck> check_;
};

TEST_F(i801SmbusTimeoutCheckTest, McbEepromNotFound) {
  // MCB EEPROM doesn't exist - check not applicable
  EXPECT_CALL(*platformFsUtils_, exists(_)).WillOnce(Return(false));

  auto result = check_->run();

  EXPECT_EQ(result.checkType(), CheckType::I801_SMBUS_TIMEOUT_CHECK);
  EXPECT_EQ(result.status(), CheckStatus::OK);
}

// Note: We don't test the successful EEPROM read path here because it would
// require mocking FbossEepromInterface which has complex constructor
// requirements. The failure paths (which are the important cases for this
// check) are tested below.

TEST_F(i801SmbusTimeoutCheckTest, DriverNotFound) {
  // MCB EEPROM exists
  EXPECT_CALL(*platformFsUtils_, exists(_))
      .WillOnce(Return(true)) // MCB EEPROM path
      .WillOnce(Return(false)); // Driver path

  // EEPROM read fails
  EXPECT_CALL(*check_, createEepromInterface(_, 0))
      .WillOnce(Throw(std::runtime_error("Failed to read EEPROM")));

  auto result = check_->run();

  EXPECT_EQ(result.checkType(), CheckType::I801_SMBUS_TIMEOUT_CHECK);
  EXPECT_EQ(result.status(), CheckStatus::OK);
}

TEST_F(i801SmbusTimeoutCheckTest, PciDeviceNotBound) {
  // MCB EEPROM exists
  EXPECT_CALL(*platformFsUtils_, exists(_))
      .WillOnce(Return(true)) // MCB EEPROM path
      .WillOnce(Return(true)) // Driver path
      .WillOnce(Return(false)); // PCI device path

  // EEPROM read fails
  EXPECT_CALL(*check_, createEepromInterface(_, 0))
      .WillOnce(Throw(std::runtime_error("Failed to read EEPROM")));

  auto result = check_->run();

  EXPECT_EQ(result.checkType(), CheckType::I801_SMBUS_TIMEOUT_CHECK);
  EXPECT_EQ(result.status(), CheckStatus::OK);
}

TEST_F(i801SmbusTimeoutCheckTest, TimeoutDetected) {
  // MCB EEPROM exists
  EXPECT_CALL(*platformFsUtils_, exists(_))
      .WillOnce(Return(true)) // MCB EEPROM path
      .WillOnce(Return(true)) // Driver path
      .WillOnce(Return(true)); // PCI device path

  // EEPROM read fails
  EXPECT_CALL(*check_, createEepromInterface(_, 0))
      .WillOnce(Throw(std::runtime_error("Failed to read EEPROM")));

  // hexdump fails with timeout error
  EXPECT_CALL(*platformUtils_, runCommand(_))
      .WillOnce(Return(
          std::make_pair(
              1,
              "hexdump: /run/devmap/eeproms/MCB_EEPROM: Connection timed out")));

  auto result = check_->run();

  EXPECT_EQ(result.checkType(), CheckType::I801_SMBUS_TIMEOUT_CHECK);
  EXPECT_EQ(result.status(), CheckStatus::PROBLEM);
  EXPECT_TRUE(result.errorMessage().has_value());
  EXPECT_TRUE(
      result.errorMessage()->find("MCB EEPROM read timed out") !=
      std::string::npos);
  EXPECT_EQ(result.remediation().value(), RemediationType::MANUAL_REMEDIATION);
}

TEST_F(i801SmbusTimeoutCheckTest, OtherHexdumpError) {
  // MCB EEPROM exists
  EXPECT_CALL(*platformFsUtils_, exists(_))
      .WillOnce(Return(true)) // MCB EEPROM path
      .WillOnce(Return(true)) // Driver path
      .WillOnce(Return(true)); // PCI device path

  // EEPROM read fails
  EXPECT_CALL(*check_, createEepromInterface(_, 0))
      .WillOnce(Throw(std::runtime_error("Failed to read EEPROM")));

  // hexdump fails with a different error (not timeout)
  EXPECT_CALL(*platformUtils_, runCommand(_))
      .WillOnce(Return(std::make_pair(1, "hexdump: permission denied")));

  auto result = check_->run();

  EXPECT_EQ(result.checkType(), CheckType::I801_SMBUS_TIMEOUT_CHECK);
  EXPECT_EQ(result.status(), CheckStatus::PROBLEM);
  EXPECT_TRUE(result.errorMessage().has_value());
  EXPECT_TRUE(
      result.errorMessage()->find("i801_smbus timeout not detected") !=
      std::string::npos);
}

TEST_F(i801SmbusTimeoutCheckTest, HexdumpSucceedsButEepromFailed) {
  // MCB EEPROM exists
  EXPECT_CALL(*platformFsUtils_, exists(_))
      .WillOnce(Return(true)) // MCB EEPROM path
      .WillOnce(Return(true)) // Driver path
      .WillOnce(Return(true)); // PCI device path

  // EEPROM read fails
  EXPECT_CALL(*check_, createEepromInterface(_, 0))
      .WillOnce(Throw(std::runtime_error("Failed to read EEPROM")));

  // hexdump succeeds
  EXPECT_CALL(*platformUtils_, runCommand(_))
      .WillOnce(Return(std::make_pair(0, "0000000 1234 5678\n")));

  auto result = check_->run();

  EXPECT_EQ(result.checkType(), CheckType::I801_SMBUS_TIMEOUT_CHECK);
  EXPECT_EQ(result.status(), CheckStatus::PROBLEM);
  EXPECT_TRUE(result.errorMessage().has_value());
  EXPECT_TRUE(
      result.errorMessage()->find("i801_smbus timeout not detected") !=
      std::string::npos);
}
