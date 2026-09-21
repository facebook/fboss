#include <fcntl.h>
#include <folly/File.h>
#include <folly/FileUtil.h>
#include <folly/ScopeGuard.h>
#include <folly/String.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/fw_util/fw_util_helpers.h"

namespace facebook::fboss::platform::fw_util {

namespace {
// Securely (re)create a fresh root-owned regular file at `path`, defending
// against symlink TOCTOU: refuse to follow or reuse a planted symlink or a
// file we do not own. `path` is built from a caller-influenced, enumerable name
// (the fpd target), so reject any parent-directory traversal first: O_NOFOLLOW
// only blocks a symlink at the final path component, not a `..` segment earlier
// in the path. Callers build the path under a fixed root (e.g. /tmp), so
// rejecting `..` keeps a crafted name from escaping that root. Returns an
// owning folly::File so the descriptor is closed on every exit path.
folly::File secureCreateRootTmpFile(const std::string& path) {
  if (path.find("..") != std::string::npos) {
    throw std::runtime_error(
        "Refusing to create temp file with '..' in path: " + path);
  }
  // If a stale file from a previous run exists, only remove it after
  // lstat-confirming it is a regular file owned by root. Never follow or
  // unlink a symlink or a file we don't own.
  struct stat st{};
  if (lstat(path.c_str(), &st) == 0) {
    if (S_ISREG(st.st_mode) && st.st_uid == 0) {
      if (unlink(path.c_str()) != 0) {
        throw std::runtime_error(
            "Failed to remove stale temp file " + path + ": " +
            folly::errnoStr(errno));
      }
    } else {
      throw std::runtime_error(
          "Refusing to use unsafe pre-existing path " + path +
          " (not a root-owned regular file)");
    }
  }
  // O_EXCL|O_NOFOLLOW guarantee we create a brand new file and never follow a
  // symlink planted between the lstat above and this open.
  int fd = open(
      path.c_str(),
      O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW,
      S_IRUSR | S_IWUSR);
  if (fd < 0) {
    throw std::runtime_error(
        "Failed to securely create temp file " + path + ": " +
        folly::errnoStr(errno));
  }
  return folly::File(fd, /*ownsFd=*/true);
}
} // namespace

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
    const std::string customContentFileName =
        "/tmp/" + fpd + "_custom_content.bin";
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
  flashromCmd.emplace_back("-p");
  flashromCmd.push_back(programmerIdentifier);

  if (flashromConfig.chip().has_value()) {
    spiChip_[fpd] = *flashromConfig.chip();
    std::string chip = detectFlashromChip(flashromConfig, fpd);
    flashromCmd.emplace_back("-c");
    flashromCmd.push_back(chip);
  }
}

void FwUtilImpl::addLayoutFile(
    const FlashromConfig& flashromConfig,
    std::vector<std::string>& flashromCmd,
    const std::string& fpd) {
  if (flashromConfig.spi_layout().has_value()) {
    // fw_util runs as root and the path is predictable, so an unprivileged user
    // could pre-create it as a symlink or escape /tmp via the fpd name. The
    // symlink-safe helper validates and confines the path and returns an owning
    // fd; remove the partial file if anything below fails.
    std::string tmpFile = "/tmp/" + fpd + "_spi_layout";
    folly::File outFile = secureCreateRootTmpFile(tmpFile);
    auto removeOnError = folly::makeGuard([&] { unlink(tmpFile.c_str()); });

    const std::string& layout = *flashromConfig.spi_layout();
    if (folly::writeFull(outFile.fd(), layout.data(), layout.size()) !=
        static_cast<ssize_t>(layout.size())) {
      throw std::runtime_error(
          "Failed to write SPI layout file " + tmpFile + ": " +
          folly::errnoStr(errno));
    }
    // Surface deferred write errors that some filesystems only report at close.
    outFile.close();
    removeOnError.dismiss();
    XLOG(INFO, "Created temporary file for SPI layout");

    // Add the layout file to the command
    flashromCmd.emplace_back("-l");
    flashromCmd.push_back(tmpFile);
  }
}

bool FwUtilImpl::createCustomContentFile(
    const std::string& customContent,
    const int& customContentOffset,
    const std::string& customContentFileName) {
  if (!std::filesystem::exists(fwBinaryFile_)) {
    return false;
  }

  /* Use firmware image size to determine the flash chip size.
     The custom content file should have the same size as the image */
  std::uintmax_t customContentFileSize =
      std::filesystem::file_size(fwBinaryFile_);

  // fw_util runs as root and customContentFileName is a predictable /tmp path,
  // so an unprivileged user could pre-create it as a symlink or escape /tmp via
  // the fpd name. The symlink-safe helper validates and confines the path and
  // returns an owning fd; remove the partial file if anything below fails.
  folly::File outFile = secureCreateRootTmpFile(customContentFileName);
  auto removeOnError =
      folly::makeGuard([&] { unlink(customContentFileName.c_str()); });

  // Size the file to match the firmware image (resize_file equivalent).
  if (ftruncate(outFile.fd(), static_cast<off_t>(customContentFileSize)) != 0) {
    throw std::runtime_error(
        "Failed to size custom content file " + customContentFileName + ": " +
        folly::errnoStr(errno));
  }

  // Write the custom content at the requested offset.
  if (folly::pwriteFull(
          outFile.fd(),
          customContent.data(),
          customContent.size(),
          static_cast<off_t>(customContentOffset)) !=
      static_cast<ssize_t>(customContent.size())) {
    throw std::runtime_error(
        "Failed to write custom content file " + customContentFileName + ": " +
        folly::errnoStr(errno));
  }
  // Surface deferred write errors that some filesystems only report at close.
  outFile.close();
  removeOnError.dismiss();
  return true;
}

void FwUtilImpl::addFileOption(
    const std::string& operation,
    std::vector<std::string>& flashromCmd,
    std::optional<std::string>& customContent) {
  if (operation == "read") {
    flashromCmd.emplace_back("-r");
  } else if (operation == "write" || operation == "verify") {
    if (!std::filesystem::exists(fwBinaryFile_)) {
      throw std::runtime_error(
          "Firmware binary file not found: " + fwBinaryFile_);
    }
    if (operation == "write") {
      flashromCmd.emplace_back("-w");
    } else {
      flashromCmd.emplace_back("-v");
    }
  } else {
    XLOG(ERR) << "Unsupported operation: " << operation;
  }
  /* Use firmware image or custom content file based on the command description
   */
  if (customContent.has_value()) {
    flashromCmd.push_back(*customContent);
  } else {
    flashromCmd.push_back(fwBinaryFile_);
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
