#include <fstream>
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/fw_util/fw_util_helpers.h"

namespace facebook::fboss::platform::fw_util {

std::string FwUtilImpl::detectFlashromChip(
    const FlashromConfig& flashromConfig,
    const std::string& fpd) {
  bool printCommand = true;

  // Construct the command to detect the chip
  XLOG(INFO) << "Detecting chip for " << fpd;

  std::vector<std::string> cmd;

  if (flashromConfig.programmer().has_value() &&
      flashromConfig.programmer_type().has_value()) {
    cmd = {
        "/usr/bin/flashrom",
        "-p",
        *flashromConfig.programmer_type() + *flashromConfig.programmer()};
  } else if (flashromConfig.programmer_type().has_value()) {
    cmd = {"/usr/bin/flashrom", "-p", *flashromConfig.programmer_type()};
  } else {
    throw std::runtime_error(
        "No programmer or programmer type set for " + fpd +
        "while detecting chip");
  }
  auto [exitStatus, standardOut] =
      PlatformUtils().runCommand(cmd, printCommand);

  /*
   * For some platform, we have to determine the type of the chip
   * using the command flashrom -p internal ... which will always return
   * a non zero value which is expected bc the command is not complete.
   * Therefore, checking the command status for preUpgrade purpose
   * (to determine the chip) will usually fail
   * for some platform as expected. so we will skip it here.
   * */

  // Pipe the output to grep and check if the chip is present
  for (const auto& chip : spiChip_[fpd]) {
    std::vector<std::string> grepCmd = {"/bin/grep", "-q", chip};
    auto [exitStatus2, standardOut2] =
        PlatformUtils().runCommandWithStdin(grepCmd, standardOut, printCommand);
    if (exitStatus2 == 0) {
      // Chip detected, return the chip name
      XLOG(INFO) << "Detected chip: " << chip;
      return chip;
    }
  }
  // If no chip is found, throw an exception or perform further actions
  XLOG(ERR) << "No chip found";
  return "";
}

void FwUtilImpl::addCommonFlashromArgs(
    const FlashromConfig& flashromConfig,
    const std::string& fpd,
    const std::string& operation,
    std::vector<std::string>& flashromCmd) {
  // layout file will be added to the command if present
  addLayoutFile(flashromConfig, flashromCmd, fpd);

  // Add extra arguments if present
  if (flashromConfig.flashromExtraArgs().has_value()) {
    for (const auto& arg : *flashromConfig.flashromExtraArgs()) {
      flashromCmd.push_back(arg);
    }
  }

  // Add file option
  addFileOption(operation, flashromCmd);
}

void FwUtilImpl::setProgrammerAndChip(
    const FlashromConfig& flashromConfig,
    const std::string& fpd,
    std::vector<std::string>& flashromCmd) {
  std::string programmerIdentifier;

  if (flashromConfig.programmer().has_value() &&
      flashromConfig.programmer_type().has_value()) {
    programmerIdentifier =
        *flashromConfig.programmer_type() + *flashromConfig.programmer();
  } else {
    // If no programmer is set, use internal programmer
    programmerIdentifier = "internal";
  }
  flashromCmd.push_back("-p");
  flashromCmd.push_back(programmerIdentifier);

  if (flashromConfig.chip().has_value()) {
    spiChip_[fpd] = *flashromConfig.chip();
    std::string chip = detectFlashromChip(flashromConfig, fpd);
    flashromCmd.push_back("-c");
    flashromCmd.push_back(chip);
  }
}

void FwUtilImpl::addLayoutFile(
    const FlashromConfig& flashromConfig,
    std::vector<std::string>& flashromCmd,
    const std::string& fpd) {
  if (flashromConfig.spi_layout().has_value()) {
    // Create a temporary file to store the SPI layout
    std::string tmpFile = "/tmp/" + fpd + "_spi_layout";
    std::ofstream outputFile(tmpFile);
    outputFile << *flashromConfig.spi_layout();
    outputFile.close();
    XLOG(INFO, "Created temporary file for SPI layout");

    // Add the layout file to the command
    flashromCmd.push_back("-l");
    flashromCmd.push_back(tmpFile);
  }
}

void FwUtilImpl::addFileOption(
    const std::string& operation,
    std::vector<std::string>& flashromCmd) {
  if (operation == "read") {
    flashromCmd.push_back("-r");
  } else if (operation == "write" || operation == "verify") {
    if (!std::filesystem::exists(FLAGS_fw_binary_file)) {
      throw std::runtime_error(
          "Firmware binary file not found: " + FLAGS_fw_binary_file);
    }
    if (operation == "write") {
      flashromCmd.push_back("-w");
    } else {
      flashromCmd.push_back("-v");
    }
  } else {
    XLOG(ERR) << "Unsupported operation: " << operation;
  }
  flashromCmd.push_back(FLAGS_fw_binary_file);
}

void FwUtilImpl::performFlashromUpgrade(
    const FlashromConfig& flashromConfig,
    const std::string& fpd) {
  bool printCommand = true;
  std::vector<std::string> flashromCmd = {"/usr/bin/flashrom"};

  setProgrammerAndChip(flashromConfig, fpd, flashromCmd);
  addCommonFlashromArgs(flashromConfig, fpd, "write", flashromCmd);

  // Execute the flashrom command
  auto [exitStatus, standardOut] =
      PlatformUtils().runCommand(flashromCmd, printCommand);
  checkCmdStatus(flashromCmd, exitStatus);
  XLOG(INFO) << standardOut;
}

void FwUtilImpl::performFlashromRead(
    const FlashromConfig& flashromConfig,
    const std::string& fpd) {
  bool printCommand = true;

  std::vector<std::string> flashromCmd = {"/usr/bin/flashrom"};

  setProgrammerAndChip(flashromConfig, fpd, flashromCmd);
  addCommonFlashromArgs(flashromConfig, fpd, "read", flashromCmd);

  // Execute the flashrom command
  auto [exitStatus, standardOut] =
      PlatformUtils().runCommand(flashromCmd, printCommand);
  checkCmdStatus(flashromCmd, exitStatus);
  XLOG(INFO) << standardOut;
}

void FwUtilImpl::performFlashromVerify(
    const FlashromConfig& flashromConfig,
    const std::string& fpd) {
  bool printCommand = true;

  std::vector<std::string> flashromCmd = {"/usr/bin/flashrom"};
  setProgrammerAndChip(flashromConfig, fpd, flashromCmd);
  addCommonFlashromArgs(flashromConfig, fpd, "verify", flashromCmd);
  // Execute the flashrom command
  auto [exitStatus, standardOut] =
      PlatformUtils().runCommand(flashromCmd, printCommand);
  checkCmdStatus(flashromCmd, exitStatus);
  XLOG(INFO) << standardOut;
}

} // namespace facebook::fboss::platform::fw_util
