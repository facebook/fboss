/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/session/ConfigSession.h"

#include <boost/filesystem.hpp>
#include <fcntl.h>
#include <fmt/format.h>
#include <folly/FileUtil.h>
#include <folly/Likely.h>
#include <folly/String.h>
#include <folly/json/json.h>
#include <glog/logging.h>
#include <pwd.h>
#include <re2/re2.h>
#include <sys/types.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <utility>
#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/PortMap.h"

namespace fs = std::filesystem;

namespace facebook::fboss {

namespace { // anonymous namespace

/*
 * Atomically update a symlink to point to a new target.
 * This creates a temporary symlink and then atomically renames it over the
 * existing symlink, ensuring there's no window where the symlink doesn't exist.
 *
 * @param symlinkPath The path to the symlink to update
 * @param newTarget The new target for the symlink
 * @throws std::runtime_error if the operation fails
 */
void atomicSymlinkUpdate(
    const std::string& symlinkPath,
    const std::string& newTarget) {
  std::error_code ec;
  fs::path symlinkFsPath(symlinkPath);

  // Generate a unique temporary path in the same directory as the target
  // symlink, we'll then atomically rename it to the final symlink name.
  std::string tmpLinkName =
      fmt::format("fboss2_tmp_{}", boost::filesystem::unique_path().string());
  fs::path tempSymlinkPath = symlinkFsPath.parent_path() / tmpLinkName;

  // Create new symlink with temporary name
  fs::create_symlink(newTarget, tempSymlinkPath, ec);
  if (ec) {
    throw std::runtime_error(
        fmt::format(
            "Failed to create temporary symlink {} to {}: {}",
            tempSymlinkPath.string(),
            newTarget,
            ec.message()));
  }

  // Atomically replace the old symlink with the new one
  fs::rename(tempSymlinkPath, symlinkPath, ec);
  if (ec) {
    // Clean up temp symlink
    fs::remove(tempSymlinkPath);
    throw std::runtime_error(
        fmt::format(
            "Failed to atomically update symlink {}: {}",
            symlinkPath,
            ec.message()));
  }
}

/*
 * Atomically create the next revision file for a given prefix.
 * This function finds the next available revision number (starting from 1)
 * and atomically creates a file with that revision number using O_CREAT|O_EXCL.
 * This ensures that concurrent commits will get different revision numbers.
 *
 * @param pathPrefix The path prefix (e.g., "/etc/coop/cli/agent")
 * @return A pair containing the path to the newly created revision file
 *         (e.g., "/etc/coop/cli/agent-r1.conf") and the revision number
 * @throws std::runtime_error if unable to create a revision file after many
 * attempts
 */
std::pair<std::string, int> createNextRevisionFile(
    const std::string& pathPrefix) {
  // Try up to 100000 revision numbers to handle concurrent commits
  // In practice, we should find one quickly
  for (int revision = 1; revision <= 100000; ++revision) {
    std::string revisionPath = fmt::format("{}-r{}.conf", pathPrefix, revision);

    // Try to atomically create the file with O_CREAT | O_EXCL
    // This will fail if the file already exists, ensuring atomicity
    int fd = open(revisionPath.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);

    if (fd >= 0) {
      // Successfully created the file - close it and return the path
      close(fd);
      return {revisionPath, revision};
    }

    // If errno is EEXIST, the file already exists - try the next revision
    if (errno != EEXIST) {
      // Some other error occurred
      throw std::runtime_error(
          fmt::format(
              "Failed to create revision file {}: {}",
              revisionPath,
              folly::errnoStr(errno)));
    }

    // File exists, try next revision number
  }

  throw std::runtime_error(
      "Failed to create revision file after 100000 attempts. "
      "This likely indicates a problem with the filesystem or too many revisions.");
}

std::string getUsername() {
  const char* user = std::getenv("USER");
  if (user != nullptr && !std::string(user).empty()) {
    return std::string(user);
  }

  // If USER env var is not set, get username from UID
  uid_t uid = getuid();
  struct passwd* pw = getpwuid(uid);
  if (pw == nullptr) {
    throw std::runtime_error(
        "Failed to get username: USER environment variable not set and "
        "getpwuid() failed");
  }
  return std::string(pw->pw_name);
}

std::string getHomeDirectory() {
  const char* home = std::getenv("HOME");
  if (home == nullptr || std::string(home).empty()) {
    throw std::runtime_error("HOME environment variable not set");
  }
  return std::string(home);
}

void ensureDirectoryExists(const std::string& dirPath) {
  std::error_code ec;
  fs::create_directories(dirPath, ec);
  // create_directories returns false if the directory already exists, but
  // that's not an error. Only throw if there's an actual error code set.
  if (ec) {
    throw std::runtime_error(
        fmt::format(
            "Failed to create directory {}: {}", dirPath, ec.message()));
  }
}

} // anonymous namespace

ConfigSession::ConfigSession() {
  username_ = getUsername();
  std::string homeDir = getHomeDirectory();

  // Use AgentDirectoryUtil to get the config directory path
  // getConfigDirectory() returns /etc/coop/agent, so we get the parent to get
  // /etc/coop
  AgentDirectoryUtil dirUtil;
  fs::path configDir = fs::path(dirUtil.getConfigDirectory()).parent_path();

  sessionConfigPath_ = homeDir + "/.fboss2/agent.conf";
  systemConfigPath_ = (configDir / "agent.conf").string();
  cliConfigDir_ = (configDir / "cli").string();

  initializeSession();
}

ConfigSession::ConfigSession(
    const std::string& sessionConfigPath,
    const std::string& systemConfigPath,
    const std::string& cliConfigDir)
    : sessionConfigPath_(sessionConfigPath),
      systemConfigPath_(systemConfigPath),
      cliConfigDir_(cliConfigDir) {
  username_ = getUsername();
  initializeSession();
}

namespace {
std::unique_ptr<ConfigSession>& getInstancePtr() {
  static std::unique_ptr<ConfigSession> instance;
  return instance;
}
} // namespace

ConfigSession& ConfigSession::getInstance() {
  auto& instance = getInstancePtr();
  if (!instance) {
    instance = std::make_unique<ConfigSession>();
  }
  return *instance;
}

void ConfigSession::setInstance(std::unique_ptr<ConfigSession> newInstance) {
  getInstancePtr() = std::move(newInstance);
}

std::string ConfigSession::getSessionConfigPath() const {
  return sessionConfigPath_;
}

std::string ConfigSession::getSystemConfigPath() const {
  return systemConfigPath_;
}

std::string ConfigSession::getCliConfigDir() const {
  return cliConfigDir_;
}

bool ConfigSession::sessionExists() const {
  return fs::exists(sessionConfigPath_);
}

cfg::AgentConfig& ConfigSession::getAgentConfig() {
  if (!configLoaded_) {
    loadConfig();
  }
  return agentConfig_;
}

const cfg::AgentConfig& ConfigSession::getAgentConfig() const {
  if (!configLoaded_) {
    throw std::runtime_error(
        "Config not loaded yet. Call getAgentConfig() (non-const) first.");
  }
  return agentConfig_;
}

utils::PortMap& ConfigSession::getPortMap() {
  if (!configLoaded_) {
    loadConfig();
  }
  return *portMap_;
}

const utils::PortMap& ConfigSession::getPortMap() const {
  if (!configLoaded_) {
    throw std::runtime_error(
        "Config not loaded yet. Call getPortMap() (non-const) first.");
  }
  return *portMap_;
}

void ConfigSession::saveConfig() {
  if (!configLoaded_) {
    throw std::runtime_error("No config loaded to save");
  }

  // We need to do a round-trip through serialize -> parse -> toPrettyJson
  // because SimpleJSONSerializer handles Thrift maps with integer keys
  // (like clientIdToAdminDistance) by converting them to strings.
  // If we use facebook::thrift::to_dynamic() directly, the integer keys
  // are preserved as integers in the folly::dynamic object, which causes
  // folly::toPrettyJson() to fail because JSON objects require string keys.
  std::string json =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(
          agentConfig_);
  std::string prettyJson = folly::toPrettyJson(folly::parseJson(json));

  // Use folly::writeFileAtomic with sync to avoid race conditions when multiple
  // threads/processes write to the same session file. WITH_SYNC ensures data
  // is flushed to disk before the atomic rename, preventing readers from
  // seeing partial/corrupted data.
  folly::writeFileAtomic(
      sessionConfigPath_, prettyJson, 0644, folly::SyncType::WITH_SYNC);
}

int ConfigSession::extractRevisionNumber(const std::string& filenameOrPath) {
  // Extract just the filename if a full path was provided
  std::string filename = filenameOrPath;
  size_t lastSlash = filenameOrPath.rfind('/');
  if (lastSlash != std::string::npos) {
    filename = filenameOrPath.substr(lastSlash + 1);
  }

  // Pattern: agent-rN.conf where N is a positive integer
  // Using RE2 instead of std::regex to avoid stack overflow issues (GCC bug)
  static const re2::RE2 pattern(R"(^agent-r(\d+)\.conf$)");
  int revision = -1;

  if (re2::RE2::FullMatch(filename, pattern, &revision)) {
    return revision;
  }
  return -1;
}

void ConfigSession::loadConfig() {
  std::string configJson;
  if (!folly::readFile(sessionConfigPath_.c_str(), configJson)) {
    throw std::runtime_error(
        fmt::format("Failed to read config file: {}", sessionConfigPath_));
  }

  apache::thrift::SimpleJSONSerializer::deserialize<cfg::AgentConfig>(
      configJson, agentConfig_);

  // Handle the legacy case where config might be a bare SwitchConfig
  if (*agentConfig_.sw() == cfg::SwitchConfig()) {
    apache::thrift::SimpleJSONSerializer::deserialize<cfg::SwitchConfig>(
        configJson, *agentConfig_.sw());
  }
  portMap_ = std::make_unique<utils::PortMap>(agentConfig_);
  configLoaded_ = true;
}

void ConfigSession::initializeSession() {
  if (!sessionExists()) {
    // Ensure the parent directory of the session config exists
    fs::path sessionPath(sessionConfigPath_);
    ensureDirectoryExists(sessionPath.parent_path().string());
    copySystemConfigToSession();
  }
}

void ConfigSession::copySystemConfigToSession() {
  // Resolve symlink if system config is a symlink
  std::string sourceConfig = systemConfigPath_;
  std::error_code ec;

  if (LIKELY(fs::is_symlink(systemConfigPath_, ec))) {
    sourceConfig = fs::read_symlink(systemConfigPath_, ec).string();
    if (ec) {
      throw std::runtime_error(
          fmt::format(
              "Failed to read symlink {}: {}",
              systemConfigPath_,
              ec.message()));
    }
    // If the symlink is relative, make it absolute relative to the system
    // config directory
    if (!fs::path(sourceConfig).is_absolute()) {
      fs::path systemConfigDir = fs::path(systemConfigPath_).parent_path();
      sourceConfig = (systemConfigDir / sourceConfig).string();
    }
  }

  // Read source config and write atomically to session config
  // This ensures that readers never see a partially written file - they either
  // see the old file or the new file, never a mix.
  // WITH_SYNC ensures data is flushed to disk before the atomic rename.
  std::string configData;
  if (!folly::readFile(sourceConfig.c_str(), configData)) {
    throw std::runtime_error(
        fmt::format("Failed to read config from {}", sourceConfig));
  }

  folly::writeFileAtomic(
      sessionConfigPath_, configData, 0644, folly::SyncType::WITH_SYNC);
}

int ConfigSession::commit(const HostInfo& hostInfo) {
  if (!sessionExists()) {
    throw std::runtime_error(
        "No config session exists. Make a config change first.");
  }

  ensureDirectoryExists(cliConfigDir_);

  // Atomically create the next revision file
  // This ensures concurrent commits get different revision numbers
  auto [targetConfigPath, revision] =
      createNextRevisionFile(fmt::format("{}/agent", cliConfigDir_));
  std::error_code ec;

  // Read the old symlink target for rollback if needed
  std::string oldSymlinkTarget;
  if (!fs::is_symlink(systemConfigPath_)) {
    throw std::runtime_error(
        fmt::format(
            "{} is not a symlink. Expected it to be a symlink.",
            systemConfigPath_));
  }
  oldSymlinkTarget = fs::read_symlink(systemConfigPath_, ec);
  if (ec) {
    throw std::runtime_error(
        fmt::format(
            "Failed to read symlink {}: {}", systemConfigPath_, ec.message()));
  }

  // Copy session config to the atomically-created revision file
  // Overwrite the empty file that was created by createNextRevisionFile
  fs::copy_file(
      sessionConfigPath_,
      targetConfigPath,
      fs::copy_options::overwrite_existing,
      ec);
  if (ec) {
    // Clean up the revision file we created
    fs::remove(targetConfigPath);
    throw std::runtime_error(
        fmt::format(
            "Failed to copy session config to {}: {}",
            targetConfigPath,
            ec.message()));
  }

  // Atomically update the symlink to point to the new config
  atomicSymlinkUpdate(systemConfigPath_, targetConfigPath);

  // Reload the config - if this fails, rollback the symlink atomically
  try {
    auto client =
        utils::createClient<apache::thrift::Client<facebook::fboss::FbossCtrl>>(
            hostInfo);
    client->sync_reloadConfig();
    LOG(INFO) << "Config committed as revision r" << revision;
  } catch (const std::exception& ex) {
    // Rollback: atomically restore the old symlink
    try {
      atomicSymlinkUpdate(systemConfigPath_, oldSymlinkTarget);
    } catch (const std::exception& rollbackEx) {
      // If rollback also fails, include both errors in the message
      throw std::runtime_error(
          fmt::format(
              "Failed to reload config: {}. Additionally, failed to rollback the config: {}",
              ex.what(),
              rollbackEx.what()));
    }
    throw std::runtime_error(
        fmt::format(
            "Failed to reload config, config was rolled back automatically: {}",
            ex.what()));
  }

  // Only remove the session config after everything succeeded
  fs::remove(sessionConfigPath_, ec);
  if (ec) {
    // Log warning but don't fail - the commit succeeded
    LOG(WARNING) << fmt::format(
        "Failed to remove session config {}: {}",
        sessionConfigPath_,
        ec.message());
  }

  return revision;
}

} // namespace facebook::fboss
