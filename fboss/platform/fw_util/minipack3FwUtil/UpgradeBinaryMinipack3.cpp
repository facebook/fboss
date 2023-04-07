//  Copyright 2021-present Facebook. All Rights Reserved.

#include "fboss/platform/fw_util/minipack3FwUtil/UpgradeBinaryMinipack3.h"

#include <filesystem>
#include <iostream>

#include <gflags/gflags.h>

DEFINE_bool(h, false, "Help");

namespace facebook::fboss::platform::fw_util {

void UpgradeBinaryMinipack3::setSpiControllerReg(std::string regAddress ) {
 //TBD,use dev_map

}

void UpgradeBinaryMinipack3::setSpiMux(std::string upgradable_components) {
  if (upgradable_components == "iob_fpga") {
    setSpiControllerReg(FBIOB_SPI_REG_SPI_FUNCTION_CTRL(IOB_FPGA_SPI_CONTROLLER_ID) )
  } else if (upgradable_components == "dom_fpga_1") {
    setSpiControllerReg(FBIOB_SPI_REG_SPI_FUNCTION_CTRL(DOM_FPGA_SPI_CONTROLLER_ID) )
  } else if (upgradable_components == "dom_fpga_2") {
    setSpiControllerReg(FBIOB_SPI_REG_SPI_FUNCTION_CTRL(DOM_FPGA_SPI_CONTROLLER_ID) )
  } else if (upgradable_components == "scm_cpld") {
    setSpiControllerReg(FBIOB_SPI_REG_SPI_FUNCTION_CTRL(SCM_CPLD_SPICONTROLLER_ID) )
  } else if (upgradable_components == "mcb_cpld") {
    setSpiControllerReg(FBIOB_SPI_REG_SPI_FUNCTION_CTRL(MCB_CPLD_SPI_CONTROLLER_ID) )
  } else if (upgradable_components == "smb_cpld") {
    setSpiControllerReg(FBIOB_SPI_REG_SPI_FUNCTION_CTRL(SMB_CPLD_SPI_CONTROLLER_ID) )
  } else if (upgradable_components == "nic_i210") {
    setSpiControllerReg(FBIOB_SPI_REG_SPI_FUNCTION_CTRL(SCM_CPLD_SPICONTROLLER_ID) )
  } else if (upgradable_components == "th5_bcm7800") {
    setSpiControllerReg(FBIOB_SPI_REG_SPI_FUNCTION_CTRL(ASIC_FW_SPI_CONTROLLER_ID) )
  } else if (upgradable_components == "bios") {
    //TBD
  }
  else
  ;
}

void UpgradeBinaryMinipack3::upgradeThroughFlashrom(
    std::string fpga,
    std::string action,
    std::string fpgaFile) {
  setSpiMux(fpga);
  flashromSpiUpgrade(fpga, action, fpgaFile);
}

std::string UpgradeBinaryMinipack3::getRegValue(std::string path) {
  std::string regVal;
  try {
    regVal = readSysfs(path);
  } catch (std::exception& e) {
    // Since we want the flow of the program to continue, we will only print a
    // message in case something happen so that we can continue with printing
    // the version information for the remaining cpld/fpgas
    std::cout << "failed to read path " << path << std::endl;
    failedPath_ = true;
  }
  return regVal;
}

std::string UpgradeBinaryMinipack3::getSysfsCpldVersion(
    std::string sysfsPath,
    std::string ver,
    std::string sub_ver) {
  std::string majorVer = getRegValue(sysfsPath + ver);
  std::string minorVer = getRegValue(sysfsPath + sub_ver);
  return folly::to<std::string>(
      std::to_string(std::stoul(majorVer, nullptr, 0)),
      ".",
      std::to_string(std::stoul(minorVer, nullptr, 0)));
}

void UpgradeBinaryMinipack3::printAllVersion(void) {
    std::cout << "IOB_FPGA:" 
              << getSysfsCpldVersion(
                iob_fpga_path, "iob_fpga_ver", "iob_fpga_sub_ver")
              << std::endl;
    std::cout << "DOM_FPGA_1:" 
              << getSysfsCpldVersion(
                dom_fpga_1_path, "dom_fpga_1_ver", "dom_fpga_1_sub_ver")
              << std::endl;
    std::cout << "DOM_FPGA_2:" 
              << getSysfsCpldVersion(
                dom_fpga_2_path, "dom_fpga_2_ver", "dom_fpga_2_sub_ver")
              << std::endl;
    std::cout << "SCM_CPLD:" 
              << getSysfsCpldVersion(
                scm_cpld_path, "scm_cpld_ver", "scm_cpld_sub_ver")
              << std::endl;
    std::cout << "MCB_CPLD:" 
              << getSysfsCpldVersion(
                mcb_cpld_path, "mcb_cpld_ver", "mcb_cpld_sub_ver")
              << std::endl;
    std::cout << "SMB_CPLD:" 
              << getSysfsCpldVersion(
                smb_cpld_path, "smb_cpld_ver", "smb_cpld_sub_ver")
              << std::endl;
    std::cout << "NIC_I210:" 
              << getSysfsCpldVersion(
                nic_i210_path, "nic_i210_ver", "nic_i210_sub_ver")
              << std::endl;
    std::cout << "TH5_BCM7800:" 
              << getSysfsCpldVersion(
                th5_bcm7800_path, "th5_bcm7800_ver", "th5_bcm7800_sub_ver")
              << std::endl;
    std::cout << "BIOS:" 
              << getSysfsCpldVersion(
                bios_path, "bios_ver", "bios_sub_ver")
              << std::endl;
}

void UpgradeBinaryMinipack3::printVersion(
    std::string binary,
    std::string upgradable_components) {
  if (binary == "all") {
    printAllVersion();
  } else if (binary == "iob_fpga") {
    std::cout << "IOB_FPGA:" 
              << getSysfsCpldVersion(
                iob_fpga_path, "iob_fpga_ver", "iob_fpga_sub_ver")
              << std::endl;
  } else if (binary == "dom_fpga_1") {
    std::cout << "DOM_FPGA_1:" 
              << getSysfsCpldVersion(
                dom_fpga_1_path, "dom_fpga_1_ver", "dom_fpga_1_sub_ver")
              << std::endl;
  } else if (binary == "dom_fpga_2") {
    std::cout << "DOM_FPGA_2:" 
              << getSysfsCpldVersion(
                dom_fpga_2_path, "dom_fpga_2_ver", "dom_fpga_2_sub_ver")
              << std::endl;
  } else if (binary == "scm_cpld") {
    std::cout << "SCM_CPLD:" 
              << getSysfsCpldVersion(
                scm_cpld_path, "scm_cpld_ver", "scm_cpld_sub_ver")
              << std::endl;
  } else if (binary == "mcb_cpld") {
    std::cout << "MCB_CPLD:" 
              << getSysfsCpldVersion(
                mcb_cpld_path, "mcb_cpld_ver", "mcb_cpld_sub_ver")
              << std::endl;
  } else if (binary == "smb_cpld") {
    std::cout << "SMB_CPLD:" 
              << getSysfsCpldVersion(
                smb_cpld_path, "smb_cpld_ver", "smb_cpld_sub_ver")
              << std::endl;
  } else if (binary == "nic_i210") {
    std::cout << "NIC_I210:" 
              << getSysfsCpldVersion(
                nic_i210_path, "nic_i210_ver", "nic_i210_sub_ver")
              << std::endl;
  } else if (binary == "th5_bcm7800") {
    std::cout << "TH5_BCM7800:" 
              << getSysfsCpldVersion(
                th5_bcm7800_path, "th5_bcm7800_ver", "th5_bcm7800_sub_ver")
              << std::endl;
  } else if (binary == "bios") {
    std::cout << "BIOS:" 
              << getSysfsCpldVersion(
                bios_path, "bios_ver", "bios_sub_ver")
              << std::endl;
  } else {
    std::cout << "unsupported binary option. please follow the usage"
              << std::endl;
    printUsage(upgradable_components);
  }
}

int UpgradeBinaryMinipack3::parseCommandLine(
    int argc,
    char* argv[],
    std::string upgradable_components) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);
  if (argc == 1 || FLAGS_h) {
    printUsage(upgradable_components);
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
    printVersion(toLower(std::string(argv[1])), upgradable_components);
  } else if (argc != 4) {
    std::cout << "missing argument" << std::endl;
    std::cout << "please follow the usage below" << std::endl;
    printUsage(upgradable_components);
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
        flashromUpgradableBinaries_.find(toLower(std::string(argv[1])));
    if (fpga != flashromUpgradableBinaries_.end()) {
      // Call function to perform upgrade/verify through flashorm argument is (fpga,
      // action, file_path)
      upgradeThroughFlashrom(
          toLower(std::string(argv[1])),
          toLower(std::string(argv[2])),
          std::string(argv[3]));
    } else {
      std::cout << "fpga file name not supported" << std::endl;
      std::cout << "please follow the usage" << std::endl;
      printUsage(upgradable_components);
    }
  } else {
    std::cout << "wrong usage. Please follow the usage" << std::endl;
    printUsage(upgradable_components);
  }

  return EX_OK;
}

} // namespace facebook::fboss::platform::fw_util
