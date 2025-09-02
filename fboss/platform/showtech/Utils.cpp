// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/showtech/Utils.h"
#include "fboss/platform/showtech/PsuHelper.h"

#include <filesystem>
#include <iostream>

#include <fmt/core.h>
#include "fboss/lib/i2c/I2cDevIo.h"

using namespace facebook::fboss::platform::showtech_config;

namespace facebook::fboss::platform {

void Utils::printHostDetails() {
  std::cout << "##### SYSTEM TIME #####" << std::endl;
  std::cout << platformUtils_.execCommand("date").second << std::endl;
  std::cout << "##### HOSTNAME #####" << std::endl;
  std::cout << platformUtils_.execCommand("hostname").second << std::endl;
  std::cout << "##### Linux Kernel Version #####" << std::endl;
  std::cout << platformUtils_.execCommand("uname -r").second << std::endl;
  std::cout << "##### UPTIME #####" << std::endl;
  std::cout << platformUtils_.execCommand("uptime").second << std::endl;
  std::cout << platformUtils_.execCommand("last reboot").second << std::endl;
}

void Utils::printFbossDetails() {
  runFbossCliCmd("product");
  runFbossCliCmd("version agent");
  runFbossCliCmd("environment sensor");
  runFbossCliCmd("environment temperature");
  runFbossCliCmd("environment fan");
  runFbossCliCmd("environment power");
}

void Utils::printWeutilDetails() {
  std::cout << "##### WEUTIL dump of all EEPROMs #####" << std::endl;
  std::cout << platformUtils_.execCommand("weutil --all").second << std::endl;
}

void Utils::printFwutilDetails() {
  std::cout << "##### FWUTIL dump of all Programmables #####" << std::endl;
  std::cout << platformUtils_
                   .execCommand(
                       "fw_util --fw_action version --fw_target_name all")
                   .second
            << std::endl;
}

void Utils::printLspciDetails() {
  std::cout << "##### LSPCI #####" << std::endl;
  std::string cmd = "lspci -vvv";
  std::cout << platformUtils_.execCommand(cmd).second << std::endl;
}

void Utils::printPortDetails() {
  runFbossCliCmd("port");
  runFbossCliCmd("fabric");
  runFbossCliCmd("lldp");
  runFbossCliCmd("interface counters");
  runFbossCliCmd("interface errors");
  runFbossCliCmd("interface flaps");
  runFbossCliCmd("interface phy");
  runFbossCliCmd("transceiver");
  if (!std::filesystem::exists("/etc/ramdisk")) {
    std::cout << "##### wedge_qsfp_util #####" << std::endl;
    auto [ret, output] =
        platformUtils_.execCommand("timeout 30 wedge_qsfp_util");
    std::cout << output << std::endl;
    if (ret == 124) {
      std::cout << "Error: wedge_qsfp_util timed out after 30 seconds"
                << std::endl;
    }
  }
}

void Utils::printSensorDetails() {
  std::cout << "##### SENSORS #####" << std::endl;
  std::cout << platformUtils_.execCommand("sensors").second << std::endl;
  std::cout << "##### Dump from sensor_service #####" << std::endl;
  std::cout << platformUtils_.execCommand("sensor_service_client").second
            << std::endl;
}

void Utils::printI2cDetails() {
  std::cout << "##### I2C Information #####" << std::endl;
  auto [ret, output] = platformUtils_.execCommand("i2cdetect -l");
  std::cout << output << std::endl;

  auto i2cBuses = i2cHelper_.getI2cBusMap();
  for (const auto& [busNum, busName] : i2cBuses) {
    if (config_.i2cBusIgnore()->contains(busName)) {
      std::cout << fmt::format("Skipping bus `i2c-{} - {}`", busNum, busName)
                << std::endl;
      continue;
    }
    auto cmd = fmt::format("time i2cdetect -y {}", busNum);
    std::cout << fmt::format("#### Running `{}` for {} ####", cmd, busName)
              << std::endl;
    std::cout << platformUtils_.execCommand(cmd).second << std::endl;
  }
}

void Utils::printPsuDetails() {
  std::cout << "##### PSU Information #####" << std::endl;

  for (const auto& psuConfig : *config_.psus()) {
    std::cout << fmt::format(
                     "#### Power Supply Slot {} Details ####",
                     *psuConfig.psuSlot())
              << std::endl;

    auto psuI2cInfo = *psuConfig.scdI2cDeviceConfig();
    int psuBusNum = i2cHelper_.findI2cBusForScdI2cDevice(
        *psuI2cInfo.scdPciAddr(), *psuI2cInfo.master(), *psuI2cInfo.bus());
    if (psuBusNum != -1) {
      try {
        PsuHelper psuHelper(psuBusNum, *psuI2cInfo.deviceAddr());
        psuHelper.dumpRegisters();
      } catch (const I2cDevIoError& ex) {
        std::cout << "Error: failed to initialize I2cDevIo for psu: "
                  << ex.what() << std::endl;
      }
    } else {
      std::cout << "Error: failed to find i2c bus for psu" << std::endl;
    }
    std::cout << std::endl;
  }
}

void Utils::runFbossCliCmd(const std::string& cmd) {
  if (!std::filesystem::exists("/etc/ramdisk")) {
    auto fullCmd = fmt::format("fboss2 show {}", cmd);
    std::cout << fmt::format("##### {} #####", fullCmd) << std::endl;
    std::cout << platformUtils_.execCommand(fullCmd).second << std::endl;
  }
}

} // namespace facebook::fboss::platform
