//  Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/platform/fw_util/upgradeBinary/upgradeBinaryDarwin.h"
#include <gflags/gflags.h>

DEFINE_bool(h, false, "Help");

using namespace facebook::fboss::platform::helpers;

namespace facebook::fboss::platform {

UpgradeBinaryDarwin::UpgradeBinaryDarwin() {
  /*Detect the bus # on master 2 bus 0 */
  int out = 0;
  const std::string cmd =
      "i2cdetect -l | grep '0000:ff:0b.3 SMBus master 2 bus 0' | cut -d '-' -f 2 | awk '{ print $1}'";
  bus = execCommand(cmd, out);
  if (out) {
    throw std::runtime_error("Error running: " + cmd);
  }
  bus.erase(std::remove(bus.begin(), bus.end(), '\n'), bus.end());
}

void UpgradeBinaryDarwin::printUsage(void) {
  std::cout << "usage:" << std::endl;
  std::cout << "fw_util <binary_name> <action> <binary_file>" << std::endl;
  std::cout << "<binary_name> : cpu_cpld, sc_scd, sc_cpld," << std::endl;
  std::cout << "sc_sat_cpld0, sc_sat_cpld1, fan_cpld, bios" << std::endl;
  std::cout << "<action> : program, verify, read" << std::endl;
}

void UpgradeBinaryDarwin::setJtagMux(std::string fpga) {
  if (fpga == "cpu_cpld") {
    i2cRegWrite(
        bus,
        std::to_string(DARWIN_CPLD_ADDR),
        std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
        0x00);
  } else if (fpga == "sc_scd") {
    i2cRegWrite(
        bus,
        std::to_string(DARWIN_CPLD_ADDR),
        std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
        0x01);
  } else if (fpga == "sc_cpld") {
    i2cRegWrite(
        bus,
        std::to_string(DARWIN_CPLD_ADDR),
        std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
        0x00);
  } else if (fpga == "sc_sat_cpld0") {
    i2cRegWrite(
        bus,
        std::to_string(DARWIN_CPLD_ADDR),
        std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
        0x02);
  } else if (fpga == "sc_sat_cpld1") {
    i2cRegWrite(
        bus,
        std::to_string(DARWIN_CPLD_ADDR),
        std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
        0x04);
  }
}
void UpgradeBinaryDarwin::upgradeThroughJam(
    std::string fpga,
    std::string action,
    std::string fpgaFile) {
  setJtagMux(fpga);
  jamUpgrade(fpga, action, fpgaFile);
  i2cRegWrite(
      bus,
      std::to_string(DARWIN_CPLD_ADDR),
      std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
      0x00);
}

void UpgradeBinaryDarwin::upgradeThroughXapp(
    std::string fpga,
    std::string action,
    std::string fpgaFile) {
  setJtagMux(fpga);
  xappUpgrade(fpga, action, fpgaFile);
  i2cRegWrite(
      bus,
      std::to_string(DARWIN_CPLD_ADDR),
      std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
      0x00);
}

int UpgradeBinaryDarwin::parseCommandLine(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);
  if (argc == 1 || FLAGS_h) {
    printUsage();
  } else if (argc != 4) {
    std::cout << "missing argument" << std::endl;
    std::cout << "please follow the usage below" << std::endl;
    printUsage();
  } else if (!isFilePresent(std::string(argv[3]))) {
    std::cout << "The file " << argv[3] << " is not present in current path"
              << std::endl;
  } else if (
      std::string(argv[2]) == std::string("program") ||
      std::string(argv[2]) == std::string("verify") ||
      std::string(argv[2]) == std::string("read")) {
    std::unordered_set<std::string>::const_iterator fpga =
        jamUpgradableBinaries.find(argv[1]);
    if (fpga != jamUpgradableBinaries.end()) {
      /* call function to perform upgrade/verify through jam
        argument is (fpga, action, file_path)
      */
      upgradeThroughJam(
          std::string(argv[1]), std::string(argv[2]), std::string(argv[3]));
    } else if (std::string(argv[1]) == std::string("sc_scd")) {
      /* call function to perform the upgrade through xapp */
      upgradeThroughXapp(
          std::string(argv[1]), std::string(argv[2]), std::string(argv[3]));
    } else if (std::string(argv[1]) == std::string("bios")) {
      /* argument is action and the filepath */
      flashromBiosUpgrade(
          std::string(argv[2]),
          std::string(argv[3]),
          chip,
          "/etc/bios_spi_layout");
    } else if (std::string(argv[1]) == std::string("fan_cpld")) {
      // TODO: Upgrade the fan_cpld through I2C
    } else {
      std::cout << "fpga file name not supported" << std::endl;
      std::cout << "please follow the usage" << std::endl;
      printUsage();
    }
  } else {
    std::cout << "wrong usage. Please follow the usage" << std::endl;
    printUsage();
  }

  return EX_OK;
}

} // namespace facebook::fboss::platform
