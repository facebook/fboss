// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "fboss/platform/platform_manager/DataStore.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_service_types.h"

namespace facebook::fboss::platform::platform_manager {

enum class ExplorationErrorType {
  I2C_DEVICE_CREATE,
  I2C_DEVICE_REG_INIT,
  I2C_DEVICE_EXPLORE,
  RUN_DEVMAP_SYMLINK,
  PCI_DEVICE_EXPLORE,
  PCI_SUB_DEVICE_CREATE_I2C_ADAPTER,
  PCI_SUB_DEVICE_CREATE_SPI_MASTER,
  PCI_SUB_DEVICE_CREATE_GPIO_CHIP,
  PCI_SUB_DEVICE_CREATE_WATCH_DOG,
  PCI_SUB_DEVICE_CREATE_FAN_CTRL,
  PCI_SUB_DEVICE_CREATE_LED_CTRL,
  PCI_SUB_DEVICE_CREATE_XCVR_CTRL,
  PCI_SUB_DEVICE_CREATE_INFO_ROM,
  PCI_SUB_DEVICE_CREATE_MISC_CTRL,
  PCI_SUB_DEVICE_CREATE_MDIO_BUS,
  IDPROM_READ,
  SLOT_PM_UNIT_ABSENCE,
  SLOT_PRESENCE_CHECK,
  // Need this for ExplorationErrorType size at compile time.
  SIZE
};

constexpr inline auto explorationErrorTypeList() {
  std::array<ExplorationErrorType, static_cast<int>(ExplorationErrorType::SIZE)>
      errorTypeList;
  for (int i = 0; i < errorTypeList.size(); i++) {
    errorTypeList[i] = static_cast<ExplorationErrorType>(i);
  }
  return errorTypeList;
};

constexpr const char* toExplorationErrorTypeStr(
    ExplorationErrorType errorType) noexcept(false) {
  switch (errorType) {
    case ExplorationErrorType::I2C_DEVICE_CREATE:
      return "i2c_device_create";
    case ExplorationErrorType::I2C_DEVICE_REG_INIT:
      return "i2c_device_reg_init";
    case ExplorationErrorType::I2C_DEVICE_EXPLORE:
      return "i2c_device_explore";
    case ExplorationErrorType::RUN_DEVMAP_SYMLINK:
      return "run_devmap_symlink";
    case ExplorationErrorType::PCI_DEVICE_EXPLORE:
      return "pci_device_explore";
    case ExplorationErrorType::PCI_SUB_DEVICE_CREATE_I2C_ADAPTER:
      return "pci_sub_device_create_i2c_adapter";
    case ExplorationErrorType::PCI_SUB_DEVICE_CREATE_SPI_MASTER:
      return "pci_sub_device_create_spi_master";
    case ExplorationErrorType::PCI_SUB_DEVICE_CREATE_GPIO_CHIP:
      return "pci_sub_device_create_gpio_chip";
    case ExplorationErrorType::PCI_SUB_DEVICE_CREATE_WATCH_DOG:
      return "pci_sub_device_create_watch_dog";
    case ExplorationErrorType::PCI_SUB_DEVICE_CREATE_FAN_CTRL:
      return "pci_sub_device_create_fan_ctrl";
    case ExplorationErrorType::PCI_SUB_DEVICE_CREATE_LED_CTRL:
      return "pci_sub_device_create_led_ctrl";
    case ExplorationErrorType::PCI_SUB_DEVICE_CREATE_XCVR_CTRL:
      return "pci_sub_device_create_xcvr_ctrl";
    case ExplorationErrorType::PCI_SUB_DEVICE_CREATE_INFO_ROM:
      return "pci_sub_device_create_info_rom";
    case ExplorationErrorType::PCI_SUB_DEVICE_CREATE_MISC_CTRL:
      return "pci_sub_device_create_misc_ctrl";
    case ExplorationErrorType::PCI_SUB_DEVICE_CREATE_MDIO_BUS:
      return "pci_sub_device_create_mdio_bus";
    case ExplorationErrorType::IDPROM_READ:
      return "idprom_read";
    case ExplorationErrorType::SLOT_PM_UNIT_ABSENCE:
      return "slot_pm_unit_absence";
    case ExplorationErrorType::SLOT_PRESENCE_CHECK:
      return "slot_presence_check";
    case ExplorationErrorType::SIZE:
      throw std::invalid_argument("Do not use ExplorationErrorType::SIZE");
  }
  __builtin_unreachable();
};

class ExplorationSummary {
 public:
  // ODS counters
  constexpr static auto kTotalFailures = "platform_explorer.total_failures";
  constexpr static auto kTotalExpectedFailures =
      "platform_explorer.total_expected_failures";
  constexpr static auto kExplorationFail = "platform_explorer.exploration_fail";
  constexpr static auto kExplorationFailByType =
      "platform_explorer.exploration_fail.{}";

  explicit ExplorationSummary(
      const PlatformConfig& config,
      const DataStore& dataStore);
  virtual ~ExplorationSummary() = default;
  void addError(
      ExplorationErrorType errorType,
      const std::string& devicePath,
      const std::string& message);
  void addError(
      ExplorationErrorType errorType,
      const std::string& slotPath,
      const std::string& deviceName,
      const std::string& errorMessage);
  // 1. Prints the exploration summary.
  // 2. Publish relevant data to ODS.
  // Return final exploration status.
  ExplorationStatus summarize();
  virtual bool isDeviceExpectedToFail(const std::string& devicePath);
  std::unordered_map<std::string, std::vector<ExplorationError>>
  getFailedDevices();

 private:
  const PlatformConfig& platformConfig_;
  const DataStore& dataStore_;
  uint nExpectedErrs_{0}, nErrs_{0};
  std::unordered_map<std::string, std::vector<ExplorationError>>
      devicePathToErrors_{}, devicePathToExpectedErrors_{};

  void print(ExplorationStatus finalStatus);
  void publishCounters(ExplorationStatus finalStatus);
};
} // namespace facebook::fboss::platform::platform_manager
