#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "fboss/platform/bsp_tests/gen-cpp2/bsp_tests_runtime_config_types.h"
namespace facebook::fboss::platform::bsp_tests {

class CdevUtils {
 public:
  // Create a new device using the FBIOB_IOC_NEW_DEVICE ioctl
  static void createNewDevice(
      const PciDeviceInfo& dev,
      const fbiob::AuxData& device,
      int id = 1);

  // Delete a device using the FBIOB_IOC_DEL_DEVICE ioctl
  static void deleteDevice(
      const PciDeviceInfo& dev,
      const fbiob::AuxData& device,
      int id = 1);

  // Make a character device path for the PCI device
  static std::string makePciPath(const PciDeviceInfo& dev);

  // Make a name for the PCI device
  static std::string makePciName(const PciDeviceInfo& dev);

  // Find PCI device directories in /sys/bus/pci/devices/
  static std::unordered_map<std::string, std::string> findPciDirs(
      const std::vector<PciDeviceInfo>& devs);

  // Check if a directory contains files matching the PCI specification
  static bool checkFilesForPci(
      const PciDeviceInfo& dev,
      const std::string& dirPath);
};

} // namespace facebook::fboss::platform::bsp_tests
