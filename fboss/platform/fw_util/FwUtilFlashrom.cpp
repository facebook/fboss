#include <fstream>
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/fw_util/fw_util_helpers.h"

namespace facebook::fboss::platform::fw_util {

std::string FwUtilImpl::detectFlashromChip(
    const FlashromConfig& flashromConfig,
    const std::string& fpd) {
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
  auto [exitStatus, standardOut] = PlatformUtils().runCommand(cmd);

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
        PlatformUtils().runCommandWithStdin(grepCmd, standardOut);
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
  std::optional<std::string> customContentFile;

  // layout file will be added to the command if present
  addLayoutFile(flashromConfig, flashromCmd, fpd);

  if (flashromConfig.custom_content().has_value() &&
      flashromConfig.custom_content_offset().has_value()) {
     /* Create a file with custom_content written at custom_content_offset.
        The flashrom command would use this file instead of the firmware image */
    const std::string customContentFileName = "/tmp/" + fpd + "_custom_content.bin";
    if (createCustomContentFile(
            *flashromConfig.custom_content(),
            *flashromConfig.custom_content_offset(),
            customContentFileName)) {
      customContentFile = customContentFileName;
    } else {
      throw std::runtime_error("Error creating custom content file for " + fpd);
    }
  }

  // Add extra arguments if present
  if (flashromConfig.flashromExtraArgs().has_value()) {
    for (const auto& arg : *flashromConfig.flashromExtraArgs()) {
      flashromCmd.push_back(arg);
    }
  }

  // Add file option
  addFileOption(operation, flashromCmd, customContentFile);
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

bool FwUtilImpl::createCustomContentFile(
    const std::string& customContent,
    const int& customContentOffset,
    const std::string& customContentFileName) {
  std::uintmax_t customContentFileSize;
  std::ofstream customContentFile;

  if (!std::filesystem::exists(FLAGS_fw_binary_file)) {
    return false;
  }

  /* Use firmware image size to determine the flash chip size.
     The custom content file should have the same size as the image */
  customContentFileSize = std::filesystem::file_size(FLAGS_fw_binary_file);
  customContentFile.open(
      customContentFileName,
      std::ofstream::out | std::ofstream::binary);
  std::filesystem::resize_file(customContentFileName, customContentFileSize);
  customContentFile.seekp(customContentOffset);
  customContentFile << customContent;
  customContentFile.close();
  return true;
}

void FwUtilImpl::addFileOption(
    const std::string& operation,
    std::vector<std::string>& flashromCmd,
    std::optional<std::string>& customContent) {
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
  /* Use firmware image or custom content file based on the command description */
  if (customContent.has_value()) {
    flashromCmd.push_back(*customContent);
  } else {
    flashromCmd.push_back(FLAGS_fw_binary_file);
  }
}

void FwUtilImpl::performFlashromUpgrade(
    const FlashromConfig& flashromConfig,
    const std::string& fpd) {
  std::vector<std::string> flashromCmd = {"/usr/bin/flashrom"};

  setProgrammerAndChip(flashromConfig, fpd, flashromCmd);
  addCommonFlashromArgs(flashromConfig, fpd, "write", flashromCmd);

  // Execute the flashrom command
  auto [exitStatus, standardOut] = PlatformUtils().runCommand(flashromCmd);
  checkCmdStatus(flashromCmd, exitStatus);
  XLOG(INFO) << standardOut;
}

void FwUtilImpl::performFlashromRead(
    const FlashromConfig& flashromConfig,
    const std::string& fpd) {
  std::vector<std::string> flashromCmd = {"/usr/bin/flashrom"};

  setProgrammerAndChip(flashromConfig, fpd, flashromCmd);
  addCommonFlashromArgs(flashromConfig, fpd, "read", flashromCmd);

  // Execute the flashrom command
  auto [exitStatus, standardOut] = PlatformUtils().runCommand(flashromCmd);
  checkCmdStatus(flashromCmd, exitStatus);
  XLOG(INFO) << standardOut;
}

void FwUtilImpl::performFlashromVerify(
    const FlashromConfig& flashromConfig,
    const std::string& fpd) {
  std::vector<std::string> flashromCmd = {"/usr/bin/flashrom"};
  setProgrammerAndChip(flashromConfig, fpd, flashromCmd);
  addCommonFlashromArgs(flashromConfig, fpd, "verify", flashromCmd);
  // Execute the flashrom command
  auto [exitStatus, standardOut] = PlatformUtils().runCommand(flashromCmd);
  checkCmdStatus(flashromCmd, exitStatus);
  XLOG(INFO) << standardOut;
}

} // namespace facebook::fboss::platform::fw_util
