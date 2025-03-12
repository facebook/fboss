// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/platformshowtech/Showtech.h"
#include "fboss/cli/fboss2/commands/show/platformshowtech/utils.h"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace facebook::fboss::show::platformshowtech {

namespace showtechUtils = show::platformshowtech::utils;

void Showtech::printVersion() {
  std::cout << "################################\n";
  std::cout << "##### SHOWTECH VERSION " << version << " #####\n";
  std::cout << "################################\n\n";
}

void Showtech::printCpuDetails() {
  std::cout << "##### CPU SYSTEM TIME #####\n"
            << showtechUtils::run_cmd_no_check("date");
  std::cout << "\n##### CPU HOSTNAME #####\n"
            << showtechUtils::run_cmd_no_check("hostname");
  std::cout << "\n##### CPU Linux Kernel Version #####\n"
            << showtechUtils::run_cmd_no_check("uname -r");
  std::cout << "\n##### CPU UPTIME #####\n"
            << showtechUtils::run_cmd_no_check("uptime") << std::endl;
}

void Showtech::printFbossDetails() {
  showtechUtils::print_fboss2_show_cmd("product");
  showtechUtils::print_fboss2_show_cmd("version agent");
  showtechUtils::print_fboss2_show_cmd("environment sensor");
  showtechUtils::print_fboss2_show_cmd("environment temperature");
  showtechUtils::print_fboss2_show_cmd("environment fan");
  showtechUtils::print_fboss2_show_cmd("environment power");
}

void Showtech::printWeutil(std::string target) {
  std::string cmd = "weutil --eeprom " + target;
  std::filesystem::path ossConfigPath{
      "/opt/fboss/share/platform_configs/weutil.json"};

  if (std::filesystem::exists(ossConfigPath)) {
    // OSS doesn't support running weutil without the -config_file arg.
    cmd = cmd + " -config_file " + ossConfigPath.string();
  }
  std::cout << "##### " + target + " SERIAL NUMBER #####\n";
  std::cout << showtechUtils::run_cmd_no_check(cmd) << std::endl;
}

void Showtech::printLspci() {
  std::string cmd;

  std::cout << "################################\n";
  std::cout << "############# LSPCI ############\n";
  std::cout << "################################\n\n";

  cmd = "lspci";
  if (verbose_) {
    cmd = cmd + " -vvv";
  }
  std::cout << cmd << std::endl
            << showtechUtils::run_cmd_no_check(cmd) << std::endl;
}

void Showtech::printI2cDetect() {
  std::string cmd;
  std::set<int> bus_to_ignore = i2cBusIgnore();
  int bus;

  std::cout << "################################\n";
  std::cout << "########## I2C DETECT ##########\n";
  std::cout << "################################\n\n";

  cmd = "i2cdetect -l";
  std::cout << cmd << std::endl
            << showtechUtils::run_cmd_no_check(cmd) << std::endl;

  for (bus = 0; bus <= showtechUtils::get_max_i2c_bus(); ++bus) {
    if (bus_to_ignore.find(bus) == bus_to_ignore.end()) {
      cmd = "i2cdetect -y " + std::to_string(bus);
      std::cout << cmd << std::endl
                << showtechUtils::run_cmd_no_check(cmd) << std::endl;
    }
  }
}

void Showtech::printLogs() {
  std::cout << "################################\n";
  std::cout << "########## DEBUG LOGS ##########\n";
  std::cout << "################################\n\n";

  std::cout << "#### SENSORS LOG ####\n";
  std::cout << showtechUtils::run_cmd_with_limit("journalctl -u sensor_service") 
            << std::endl;

  std::cout << "#### FAN LOG ####\n";
  std::cout << showtechUtils::run_cmd_with_limit("journalctl -u fan_service") 
            << std::endl;

  std::cout << "#### DATA CORRAL LOG ####\n";
  std::cout << showtechUtils::run_cmd_with_limit("journalctl -u data_corral_service")
            << std::endl;

  std::cout << "#### QSFP LOG ####\n";
  std::cout << showtechUtils::run_cmd_with_limit("journalctl -u qsfp_service") 
            << std::endl;

  std::cout << "#### SW AGENT LOG ####\n";
  std::cout << showtechUtils::run_cmd_with_limit("journalctl -u fboss_sw_agent") 
            << std::endl;

  std::cout << "#### HW AGENT LOG ####\n";
  std::cout << showtechUtils::run_cmd_with_limit("journalctl -u fboss_hw_agent@0")
            << std::endl;

  std::cout << "#### DMESG LOG ####\n";
  std::cout << showtechUtils::run_cmd_with_limit("dmesg") 
            << std::endl;

  std::cout << "#### BOOT CONSOLE LOG ####\n";
  std::cout << showtechUtils::run_cmd_with_limit("cat /var/log/boot.log") 
            << std::endl;

  std::cout << "#### LINUX MESSAGES LOG ####\n";
  std::cout << showtechUtils::run_cmd_with_limit("cat /var/log/messages") 
            << std::endl;
}

void Showtech::printL1Info() {
  std::cout << "################################\n";
  std::cout << "########### L1 LOGS ############\n";
  std::cout << "################################\n\n";

  showtechUtils::print_fboss2_show_cmd("port");
  showtechUtils::print_fboss2_show_cmd("fabric");
  showtechUtils::print_fboss2_show_cmd("lldp");
  showtechUtils::print_fboss2_show_cmd("interface counters");
  showtechUtils::print_fboss2_show_cmd("interface errors");
  showtechUtils::print_fboss2_show_cmd("interface flaps");
  showtechUtils::print_fboss2_show_cmd("interface phy");
  showtechUtils::print_fboss2_show_cmd("transceiver");

  if (verbose_) {
    std::cout << "#### wedge_qsfp_util ####\n";
    std::cout << showtechUtils::run_cmd_no_check("wedge_qsfp_util") << std::endl;
  }
}

void Showtech::printShowtech() {
  printVersion();
  printCpuDetails();
  printFbossDetails();
  printPlatformInfo();
  printLspci();
  printL1Info();
  if (verbose_) {
    printI2cDetect();
    printLogs();
  }
}

} // namespace facebook::fboss::show::platformshowtech
