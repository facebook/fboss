// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/showtech/Utils.h"

#include <gpiod.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <thread>
#include <tuple>

#include <fmt/core.h>
#include <folly/String.h>
#include <re2/re2.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/GpiodLine.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"
#include "fboss/platform/showtech/FanImpl.h"
#include "fboss/platform/showtech/PsuHelper.h"

using namespace facebook::fboss::platform::showtech_config;

namespace facebook::fboss::platform {

void Utils::printHostDetails() {
  std::cout << "##### Host Information #####" << std::endl;
  std::cout << "#### SYSTEM TIME ####" << std::endl;
  std::cout << platformUtils_.execCommand("date").second << std::endl;
  std::cout << "#### HOSTNAME ####" << std::endl;
  std::cout << platformUtils_.execCommand("hostname").second << std::endl;
  std::cout << "#### Linux Kernel Version ####" << std::endl;
  std::cout << platformUtils_.execCommand("uname -r").second << std::endl;
  std::cout << "#### UPTIME ####" << std::endl;
  std::cout << platformUtils_.execCommand("uptime").second << std::endl;
  std::cout << platformUtils_.execCommand("last reboot").second << std::endl;
}

void Utils::printFbossDetails() {
  std::cout << "##### FBOSS Information #####" << std::endl;
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
  std::cout << "##### Port Information #####" << std::endl;
  runFbossCliCmd("port");
  runFbossCliCmd("fabric");
  runFbossCliCmd("lldp");
  runFbossCliCmd("interface counters");
  runFbossCliCmd("interface errors");
  runFbossCliCmd("interface flaps");
  runFbossCliCmd("interface phy");
  runFbossCliCmd("transceiver");
  if (!std::filesystem::exists("/etc/ramdisk")) {
    std::cout << "#### wedge_qsfp_util ####" << std::endl;
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
  std::cout << "##### Sensor Information #####" << std::endl;
  std::cout << "#### SENSORS ####" << std::endl;
  std::cout << platformUtils_.execCommand("sensors").second << std::endl;
  std::cout << "#### Dump from sensor_service ####" << std::endl;
  std::cout << platformUtils_.execCommand("sensor_service_client").second
            << std::endl;
}

void Utils::printI2cDetails() {
  std::cout << "##### I2C Scan Information #####" << std::endl;
  auto [ret, output] = platformUtils_.execCommand("i2cdetect -l");
  std::cout << output << std::endl;

  auto i2cBuses = i2cHelper_.findI2cBuses();
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

void Utils::printI2cDumpDetails() {
  std::cout << "##### I2C Dump Information #####" << std::endl;
  if (config_.i2cDumpDevices()->empty()) {
    std::cout << "No device to i2cdump found from configs\n" << std::endl;
    return;
  }

  for (const auto& path : *config_.i2cDumpDevices()) {
    std::cout << fmt::format("#### i2cdump for {} ####", path) << std::endl;
    auto i2cInfo = getI2cInfoForDevice(path);
    if (i2cInfo) {
      auto [bus, devAddr] = *i2cInfo;
      auto cmd = fmt::format("timeout 15 i2cdump -f -y {} {} b", bus, devAddr);

      std::cout << fmt::format("Running `{}`", cmd) << std::endl;
      auto [ret, output] = platformUtils_.execCommand(cmd);
      std::cout << output << std::endl;
      if (ret == 124) {
        std::cout << "Error: command timed out after 15 seconds" << std::endl;
      }
    }
  }
}

void Utils::printPsuDetails() {
  std::cout << "##### PSU Information #####" << std::endl;

  for (const auto& psu : *config_.psus()) {
    std::cout << fmt::format("#### PSU Details {} ####", psu) << std::endl;

    auto i2cInfo = getI2cInfoForDevice(psu);
    if (i2cInfo) {
      auto [busNum, deviceAddr] = *i2cInfo;
      try {
        PsuHelper(busNum, deviceAddr).dumpRegisters();
      } catch (const std::exception& e) {
        std::cout << fmt::format(
                         "Error: failed to dump registers: {}", e.what())
                  << std::endl;
      }
    }
    std::cout << std::endl;
  }
}

void Utils::printGpioDetails() {
  std::cout << "##### GPIO Information #####" << std::endl;
  if (config_.gpios()->empty()) {
    std::cout << "No GPIO chip found from configs\n" << std::endl;
    return;
  }
  for (const auto& gpio : *config_.gpios()) {
    printGpio(gpio);
  }
}

void Utils::printPemDetails() {
  std::cout << "##### PEM Information #####" << std::endl;
  if (config_.pems()->empty()) {
    std::cout << "No PEM found found from config\n" << std::endl;
    return;
  }
  for (const auto& pem : *config_.pems()) {
    std::cout << fmt::format("#### {} ####", *pem.name()) << std::endl;
    printSysfsAttribute("present", *pem.presenceSysfsPath());
    printSysfsAttribute("input_ok", *pem.inputOkSysfsPath());
    printSysfsAttribute("status", *pem.statusSysfsPath());
  }
  std::cout << std::endl;
}

void Utils::printFanDetails() {
  std::cout << "##### Fan Information #####" << std::endl;
  std::string fanServiceConfJson = ConfigLib().getFanServiceConfig();
  auto fanServiceConfig = apache::thrift::SimpleJSONSerializer::deserialize<
      fan_service::FanServiceConfig>(fanServiceConfJson);
  if (fanServiceConfig.fans()->empty()) {
    std::cout << "No fans found from configs\n" << std::endl;
    return;
  }

  for (int i = 0; i < 2; ++i) {
    if (i > 0) {
      std::cout << "Sleeping for 0.5s...\n" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    for (const auto& fan : *fanServiceConfig.fans()) {
      std::string presenceStr{"ReadError"}, rpmStr{"ReadError"},
          pwmPercentStr{"ReadError"};

      FanImpl fanImpl(fan);

      try {
        presenceStr = fanImpl.readFanIsPresentOnDevice() ? "True" : "False";
      } catch (const std::exception& e) {
        std::cout << "Error in reading Fan Presence: " << e.what() << std::endl;
        continue;
      }
      try {
        rpmStr = std::to_string(fanImpl.readFanRpm());
      } catch (const std::exception& e) {
        std::cout << "Error in reading Fan RPM: " << e.what() << std::endl;
      }
      try {
        pwmPercentStr = fmt::format("{}%", fanImpl.readFanPwmPercent());
      } catch (const std::exception& e) {
        std::cout << "Error in reading Fan PWM: " << e.what() << std::endl;
      }
      std::cout << fmt::format(
                       "{} -> present={}, rpm={}, pwmPercent={}",
                       *fan.fanName(),
                       presenceStr,
                       rpmStr,
                       pwmPercentStr)
                << std::endl;
    }
    std::cout << std::endl;
  }
}

void Utils::printFanspinnerDetails() {
  std::cout << "##### Fanspinner Information #####" << std::endl;
  if (config_.fanspinners()->empty()) {
    std::cout << "No fanspinner found from config\n" << std::endl;
    return;
  }
  for (const auto& fs : *config_.fanspinners()) {
    std::cout << fmt::format("#### Fanspinner Path: {} ####", *fs.path())
              << std::endl;
    for (const auto& attr : *fs.sysfsAttributes()) {
      printSysfsAttribute(*attr.name(), *attr.path());
    }
  }
  std::cout << std::endl;
}

void Utils::printNvmeDetails() {
  std::cout << "##### Nvme Information #####" << std::endl;

  auto listCmd = "nvme list";
  auto [ret, output] = platformUtils_.execCommand(listCmd);
  if (ret != 0) {
    std::cout << fmt::format(
                     "Error: `{}` exited with non-zero status: {}\n",
                     listCmd,
                     ret)
              << std::endl;
    return;
  }

  // Parse nvme-list output for devices
  std::vector<std::string> nvmeDevices;
  std::vector<std::string> lines;
  folly::split('\n', output, lines, true);
  if (lines.size() > 2) {
    for (size_t i = 2; i < lines.size(); ++i) {
      std::vector<std::string> fields;
      folly::split(' ', lines[i], fields, true);
      if (!fields.empty()) {
        nvmeDevices.emplace_back(fields[0]);
      }
    }
  }

  if (nvmeDevices.empty()) {
    std::cout << fmt::format("No nvme device found from `{}`\n", listCmd)
              << std::endl;
    return;
  }

  std::array<std::string, 4> cmds{
      "nvme smart-log {}",
      "nvme error-log {}",
      "nvme id-ctrl {} -H",
      "nvme id-ns {} -H"};
  for (const auto& nvmeDevice : nvmeDevices) {
    for (const auto& cmdStr : cmds) {
      auto cmd = fmt::format(fmt::runtime(cmdStr), nvmeDevice);
      std::cout << fmt::format("#### Running `{}` ####", cmd) << std::endl;
      std::cout << platformUtils_.execCommand(cmd).second << std::endl;
    }
  }
}

void Utils::printPowerGoodDetails() {
  std::cout << "##### Power Good Information #####" << std::endl;

  if (config_.scPowerGood() && config_.scPowerGood()->sysfsAttribute()) {
    std::cout << "Reading scPowerGood from sysfs" << std::endl;
    auto pgSysfs = *config_.scPowerGood()->sysfsAttribute();
    printSysfsAttribute(*pgSysfs.name(), *pgSysfs.path());
  } else if (config_.scPowerGood() && config_.scPowerGood()->gpioAttribute()) {
    std::cout << "Reading scPowerGood from gpio" << std::endl;
    auto pgGpio = *config_.scPowerGood()->gpioAttribute();
    printGpio(pgGpio);
  } else {
    std::cout << "No powergood info found from config\n";
  }
  std::cout << std::endl;
}

void Utils::printServiceLogs(const std::string& service) const {
  std::string cmd;
  auto logFile = fmt::format("/var/facebook/logs/fboss/{}.log", service);
  if (std::filesystem::exists(logFile)) {
    cmd = fmt::format("cat {}", logFile);
  } else {
    cmd = fmt::format("journalctl -u {}", service);
  }
  std::cout << execCommandWithLimit(cmd).second << std::endl;
}

void Utils::printLogs() {
  std::cout << "##### Platform Manager Log #####" << std::endl;
  printServiceLogs("platform_manager");

  std::cout << "##### Sensor Service Log #####" << std::endl;
  printServiceLogs("sensor_service");

  std::cout << "##### Fan Service Log #####" << std::endl;
  printServiceLogs("fan_service");

  std::cout << "##### Data Corral Log #####" << std::endl;
  printServiceLogs("data_corral_service");

  std::cout << "##### QSFP Service Log #####" << std::endl;
  printServiceLogs("qsfp_service");

  std::cout << "##### fboss_sw_agent Log #####" << std::endl;
  printServiceLogs("fboss_sw_agent");

  std::cout << "##### fboss_hw_agent@0 Log #####" << std::endl;
  printServiceLogs("fboss_hw_agent@0");

  std::cout << "##### demsg Log #####" << std::endl;
  std::cout << execCommandWithLimit("dmesg").second << std::endl;

  std::cout << "##### Boot Console Log #####" << std::endl;
  std::cout << execCommandWithLimit("cat /var/log/boot.log").second
            << std::endl;

  std::cout << "##### Linux Messages Log #####" << std::endl;
  std::cout << execCommandWithLimit("cat /var/log/messages").second
            << std::endl;
}

void Utils::runFbossCliCmd(const std::string& cmd) {
  if (!std::filesystem::exists("/etc/ramdisk")) {
    auto fullCmd = fmt::format("fboss2 show {}", cmd);
    std::cout << fmt::format("#### {} ####", fullCmd) << std::endl;
    std::cout << platformUtils_.execCommand(fullCmd).second << std::endl;
  }
}

std::pair<int, std::string> Utils::execCommandWithLimit(
    const std::string& cmd,
    int maxLines) const {
  auto [ret, output] = platformUtils_.execCommand(cmd);
  std::vector<std::string> lines;
  folly::split('\n', output, lines);

  int totalLines = lines.size();
  if (totalLines <= maxLines) {
    return {ret, output};
  }

  // truncate output
  int headCount = (maxLines + 1) / 2;
  int tailCount = maxLines / 2;
  std::string first =
      folly::join('\n', folly::range(lines.begin(), lines.begin() + headCount));
  std::string last =
      folly::join('\n', folly::range(lines.end() - tailCount, lines.end()));

  return {
      ret,
      fmt::format(
          "=== Output exceeds {} lines (total: {}). "
          "Showing first {} and last {} lines ===\n\n"
          "{}\n\n"
          "=== {} lines truncated ===\n\n"
          "{}\n",
          maxLines,
          totalLines,
          headCount,
          tailCount,
          first,
          totalLines - maxLines,
          last)};
}

void Utils::printSysfsAttribute(
    const std::string& label,
    const std::string& path) {
  std::cout << label << ": ";
  try {
    std::cout << readSysfs(path) << std::endl;
  } catch (const std::exception& e) {
    std::cout << fmt::format(
                     "Error: failed to read sysfs path {}: {}", path, e.what())
              << std::endl;
  }
}

std::optional<std::tuple<int, int>> Utils::getI2cInfoForDevice(
    const std::string& path) {
  std::string i2cPath{};
  try {
    i2cPath = std::filesystem::read_symlink(path).string();
  } catch (const std::filesystem::filesystem_error& ex) {
    std::cout << fmt::format(
                     "Error: failed to resolve device symlink {}:{}",
                     path,
                     ex.what())
              << std::endl;
    return std::nullopt;
  }

  int busNum;
  int deviceAddr;
  RE2 i2cPattern(R"(/sys/bus/i2c/devices/(\d+)-([0-9a-fA-F]+))");

  if (!RE2::PartialMatch(i2cPath, i2cPattern, &busNum, RE2::Hex(&deviceAddr))) {
    std::cout << "Error: Could not extract i2c bus and address from path: "
              << i2cPath << std::endl;
    return std::nullopt;
  }

  std::cout << fmt::format(
                   "Extracted i2c bus: {}, device address: 0x{:04x}",
                   busNum,
                   deviceAddr)
            << std::endl;
  return std::make_tuple(busNum, deviceAddr);
}

void Utils::printGpio(const Gpio& gpio) {
  std::cout << fmt::format("#### GPIO Chip Details {} ####", *gpio.path())
            << std::endl;
  struct gpiod_chip* chip = gpiod_chip_open(gpio.path()->c_str());
  for (const auto& line : *gpio.lines()) {
    std::cout << fmt::format(
        "line {:>3}:   {:<15} -> ", *line.lineIndex(), *line.name());
    try {
      std::cout << GpiodLine(chip, *line.lineIndex(), *line.name()).getValue()
                << std::endl;
    } catch (const std::exception& e) {
      std::cout << fmt::format("Error: failed to read gpio line: {}", e.what())
                << std::endl;
    }
  }
  gpiod_chip_close(chip);
  std::cout << std::endl;
}

} // namespace facebook::fboss::platform
