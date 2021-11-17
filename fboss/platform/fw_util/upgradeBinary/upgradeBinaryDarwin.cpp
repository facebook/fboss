//  Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/platform/fw_util/upgradeBinary/upgradeBinaryDarwin.h"
#include <gflags/gflags.h>

DEFINE_bool(h, false, "Help");

using namespace facebook::fboss::platform::helpers;

namespace facebook::fboss::platform {

UpgradeBinaryDarwin::UpgradeBinaryDarwin() {
  /*Detect the sc_bus # on master 2 bus 0 */
  sc_bus = getBusNumber("2", "0");

  /* Detect the fan_bus number of master 3 bus 0 */
  fan_bus = getBusNumber("3", "0");
}

std::string UpgradeBinaryDarwin::getBusNumber(
    std::string masterNumber,
    std::string busNumber) {
  const std::string cmd = folly::to<std::string>(
      "i2cdetect -l | grep '0000:ff:0b.3 SMBus master ",
      masterNumber,
      " bus ",
      busNumber,
      "' | cut -d '-' -f 2 | awk '{ print $1}'");
  std::string bus = execCommand(cmd);
  bus.erase(std::remove(bus.begin(), bus.end(), '\n'), bus.end());
  return bus;
}
void UpgradeBinaryDarwin::printUsage(void) {
  std::cout << "usage:" << std::endl;
  std::cout << "fw_util <all|binary_name> <action> <binary_file>" << std::endl;
  std::cout << "<binary_name> : cpu_cpld, sc_scd, sc_cpld," << std::endl;
  std::cout
      << "sc_sat_cpld0, sc_sat_cpld1, fan_cpld, bios, all (only when pulling version)"
      << std::endl;
  std::cout << "<action> : program, verify, read, version" << std::endl;
  std::cout
      << "<binary_file> : path to binary file which is NOT supported when pulling fw version"
      << std::endl;
  std::cout
      << "all: only supported when pulling fw version. Ex:fw_util all version"
      << std::endl;
}

void UpgradeBinaryDarwin::setJtagMux(std::string fpga) {
  if (fpga == "cpu_cpld") {
    i2cRegWrite(
        sc_bus,
        std::to_string(DARWIN_CPLD_ADDR),
        std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
        0x00);
  } else if (fpga == "sc_scd") {
    i2cRegWrite(
        sc_bus,
        std::to_string(DARWIN_CPLD_ADDR),
        std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
        0x01);
  } else if (fpga == "sc_cpld") {
    i2cRegWrite(
        sc_bus,
        std::to_string(DARWIN_CPLD_ADDR),
        std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
        0x00);
  } else if (fpga == "sc_sat_cpld0") {
    i2cRegWrite(
        sc_bus,
        std::to_string(DARWIN_CPLD_ADDR),
        std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
        0x02);
  } else if (fpga == "sc_sat_cpld1") {
    i2cRegWrite(
        sc_bus,
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
      sc_bus,
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
      sc_bus,
      std::to_string(DARWIN_CPLD_ADDR),
      std::to_string(DARWIN_CPLD_JTAG_REG_ADDR),
      0x00);
}
std::string UpgradeBinaryDarwin::getRegValue(std::string path) {
  std::string regVal;
  try {
    regVal = readSysfs(path);
  } catch (std::exception& e) {
    /* Since we want the flow of the program to continue,
     * We will only print a message in case something happen so
     * that we can continue with printing the version information
     * for the remaining cpld/fpgas
     * */
    std::cout << "failed to read path " << path << std::endl;
    failedPath = true;
  }
  return regVal;
}

std::string UpgradeBinaryDarwin::getSysfsCpldVersion(std::string sysfsPath) {
  std::string majorVer = getRegValue(sysfsPath + "fpga_ver");
  std::string minorVer = getRegValue(sysfsPath + "fpga_sub_ver");
  return folly::to<std::string>(
      std::to_string(std::stoul(majorVer, nullptr, 0)),
      ".",
      std::to_string(std::stoul(minorVer, nullptr, 0)));
}
std::string UpgradeBinaryDarwin::getScCpldVersion(void) {
  std::string majorVer =
      i2cRegRead(sc_bus, std::to_string(DARWIN_CPLD_ADDR), "0x1");
  std::string minorVer =
      i2cRegRead(sc_bus, std::to_string(DARWIN_CPLD_ADDR), "0x0");
  return folly::to<std::string>(
      std::to_string(std::stoul(majorVer, nullptr, 0)),
      ".",
      std::to_string(std::stoul(minorVer, nullptr, 0)));
}
std::string UpgradeBinaryDarwin::getScSatCpldVersion(
    uint16_t sat_cpld_version) {
  std::string majorVer = std::to_string(sat_cpld_version >> 8);
  std::string minorVer = std::to_string(sat_cpld_version & 0x00FF);
  return folly::to<std::string>(majorVer, ".", minorVer);
}

std::string UpgradeBinaryDarwin::getFanCpldVersion(void) {
  std::string majorVer =
      i2cRegRead(fan_bus, std::to_string(DARWIN_FAN_ADDR), "0x1");
  std::string minorVer =
      i2cRegRead(fan_bus, std::to_string(DARWIN_FAN_ADDR), "0x0");
  return folly::to<std::string>(
      std::to_string(std::stoul(majorVer, nullptr, 0)),
      ".",
      std::to_string(std::stoul(minorVer, nullptr, 0)));
}
void UpgradeBinaryDarwin::constructCpuCpldPath(std::string node) {
  const std::string cmd =
      folly::to<std::string>("cp ", darwin_cpu_cpld_path, node, " /tmp/", node);
  int ret = runCmd(cmd);
  if (ret < 0) {
    throw std::runtime_error(folly::to<std::string>("Error running ", cmd));
  }
}

uint32_t UpgradeBinaryDarwin::getScdSatVersion() {
  const std::string cmd =
      "devmem2 0xfbe00400 | grep -i 0xfbe00400 | cut -d ':' -f 2";
  std::string regVal = execCommand(cmd);
  return std::stoul(regVal, nullptr, 0);
}

std::string UpgradeBinaryDarwin::getBiosVersion() {
  const std::string cmd =
      "dmidecode | grep Version | head -n 1 | cut -d ':' -f 2";
  std::string biosVersion = execCommand(cmd);
  biosVersion.erase(
      std::remove(biosVersion.begin(), biosVersion.end(), '\n'),
      biosVersion.end());
  biosVersion.erase(
      std::remove(biosVersion.begin(), biosVersion.end(), ' '),
      biosVersion.end());
  return biosVersion;
}

void UpgradeBinaryDarwin::printAllVersion(void) {
  constructCpuCpldPath("fpga_ver");
  constructCpuCpldPath("fpga_sub_ver");
  uint32_t sat_cpld_version = getScdSatVersion();

  std::cout << "BIOS:" << getBiosVersion() << std::endl;
  std::cout << "CPU_CPLD:" << getSysfsCpldVersion("/tmp/") << std::endl;
  std::cout << "SC_SCD:" << getSysfsCpldVersion(darwin_sc_scd_path)
            << std::endl;
  std::cout << "SC_CPLD:" << getScCpldVersion() << std::endl;
  std::cout << "FAN_CPLD:" << getFanCpldVersion() << std::endl;
  std::cout << "SC_SAT_CPLD_0: "
            << getScSatCpldVersion(sat_cpld_version & 0x0000FFFF) << std::endl;
  std::cout << "SC_SAT_CPLD_1: " << getScSatCpldVersion(sat_cpld_version >> 16)
            << std::endl;

  if (failedPath) {
    throw std::runtime_error("reading some path failed");
  }
}

void UpgradeBinaryDarwin::printVersion(std::string binary) {
  if (binary == "all") {
    printAllVersion();
  } else if (binary == "bios") {
    std::cout << "BIOS:" << getBiosVersion() << std::endl;
  } else if (binary == "cpu_cpld") {
    constructCpuCpldPath("fpga_ver");
    constructCpuCpldPath("fpga_sub_ver");
    std::cout << "CPU_CPLD:" << getSysfsCpldVersion("/tmp/") << std::endl;
  } else if (binary == "sc_scd") {
    std::cout << "SC_SCD:" << getSysfsCpldVersion(darwin_sc_scd_path)
              << std::endl;
  } else if (binary == "sc_cpld") {
    std::cout << "SC_CPLD:" << getScCpldVersion() << std::endl;
  } else if (binary == "fan_cpld") {
    std::cout << "FAN_CPLD:" << getFanCpldVersion() << std::endl;
  } else if (binary == "sc_sat_cpld_0") {
    uint32_t sat_cpld_version = getScdSatVersion();
    std::cout << "SC_SAT_CPLD_0: "
              << getScSatCpldVersion(sat_cpld_version & 0x0000FFFF)
              << std::endl;
  } else if (binary == "sc_sat_cpld_1") {
    uint32_t sat_cpld_version = getScdSatVersion();
    std::cout << "SC_SAT_CPLD_1: "
              << getScSatCpldVersion(sat_cpld_version >> 16) << std::endl;
  } else {
    std::cout << "unsupported binary option. please follow the usage"
              << std::endl;
    printUsage();
  }
}

int UpgradeBinaryDarwin::parseCommandLine(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);
  if (argc == 1 || FLAGS_h) {
    printUsage();
  } else if (
      argc >= 3 && std::string(argv[2]) == toLower(std::string("version"))) {
    if (argc > 3) {
      std::cout << std::string(argv[3]) << " cannot be part of version command"
                << std::endl;
      std::cout
          << "To pull firmware version, please enter the following command"
          << std::endl;
      std::cout << "fw_util <all|binary_name> <action>" << std::endl;
    }
    printVersion(toLower(std::string(argv[1])));
  } else if (argc != 4) {
    std::cout << "missing argument" << std::endl;
    std::cout << "please follow the usage below" << std::endl;
    printUsage();
  } else if (
      toLower(std::string(argv[2])) == std::string("write") &&
      !isFilePresent(std::string(argv[3]))) {
    std::cout << "The file " << argv[3] << " is not present in current path"
              << std::endl;
  } else if (
      toLower(std::string(argv[2])) == std::string("program") ||
      toLower(std::string(argv[2])) == std::string("verify") ||
      toLower(std::string(argv[2])) == std::string("read")) {
    std::unordered_set<std::string>::const_iterator fpga =
        jamUpgradableBinaries.find(toLower(std::string(argv[1])));
    if (fpga != jamUpgradableBinaries.end()) {
      /* call function to perform upgrade/verify through jam
        argument is (fpga, action, file_path)
      */
      upgradeThroughJam(
          toLower(std::string(argv[1])),
          toLower(std::string(argv[2])),
          toLower(std::string(argv[3])));
    } else if (toLower(std::string(argv[1])) == std::string("sc_scd")) {
      /* call function to perform the upgrade through xapp */
      upgradeThroughXapp(
          toLower(std::string(argv[1])),
          toLower(std::string(argv[2])),
          toLower(std::string(argv[3])));
    } else if (toLower(std::string(argv[1])) == std::string("bios")) {
      /* argument is action and the filepath */
      flashromBiosUpgrade(
          toLower(std::string(argv[2])),
          toLower(std::string(argv[3])),
          chip,
          "/etc/bios_spi_layout");
    } else if (toLower(std::string(argv[1])) == std::string("fan_cpld")) {
      // TODO: Upgrade the fan_cpld through I2C
      std::cout << "upgrade not supported for fan_cpld yet" << std::endl;
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
