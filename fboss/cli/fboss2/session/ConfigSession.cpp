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
#include <folly/Subprocess.h>
#include <folly/json/json.h>
#include <glog/logging.h>
#include <pwd.h>
#include <re2/re2.h>
#include <sys/types.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <utility>
#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
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

/*
 * Get the current revision number by reading the symlink target.
 * Returns -1 if unable to determine the current revision.
 */
int getCurrentRevisionNumber(const std::string& systemConfigPath) {
  std::error_code ec;

  if (!fs::is_symlink(systemConfigPath, ec)) {
    return -1;
  }

  std::string target = fs::read_symlink(systemConfigPath, ec);
  if (ec) {
    return -1;
  }

  return ConfigSession::extractRevisionNumber(target);
}

} // anonymous namespace

/*
 * Read the command line from /proc/self/cmdline, skipping argv[0].
 * Returns the command arguments as a space-separated string,
 * e.g., "config interface eth1/1/1 mtu 9000"
 */
std::string ConfigSession::readCommandLineFromProc() const {
  std::ifstream file("/proc/self/cmdline");
  if (!file) {
    throw std::runtime_error(
        fmt::format(
            "Failed to open /proc/self/cmdline: {}", folly::errnoStr(errno)));
  }

  std::vector<std::string> args;
  std::string arg;
  bool first = true;
  while (std::getline(file, arg, '\0')) {
    if (first) {
      // Skip argv[0] (program name)
      first = false;
      continue;
    }
    if (!arg.empty()) {
      args.push_back(arg);
    }
  }
  return folly::join(" ", args);
}

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

void ConfigSession::saveConfig(
    cli::ServiceType service,
    cli::ConfigActionLevel actionLevel) {
  if (!configLoaded_) {
    throw std::runtime_error("No config loaded to save");
  }

  // We need to do a round-trip through serialize -> parse -> toPrettyJson
  // because SimpleJSONSerializer handles Thrift maps with integer keys
  // (like clientIdToAdminDistance) by converting them to strings.
  // If we use facebook::thrift::to_dynamic() directly, the integer keys
  // are preserved as integers in the folly::dynamic object, which causes
  // folly::toPrettyJson() to fail because JSON objects requires string keys.
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

  // Automatically record the command from /proc/self/cmdline.
  // This ensures all config commands are tracked without requiring manual
  // instrumentation in each command implementation.
  std::string rawCmd = readCommandLineFromProc();
  CHECK(!rawCmd.empty())
      << "saveConfig() called with no command line arguments";
  // Only record if this is a config command and not already the last one
  // recorded as that'd be idempotent anyway.
  if (rawCmd.find("config ") == 0 &&
      (commands_.empty() || commands_.back() != rawCmd)) {
    commands_.push_back(rawCmd);
  }

  // Update the required action metadata for this service
  updateRequiredAction(service, actionLevel);

  // Save command history and action levels to metadata
  saveMetadata();
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

std::string ConfigSession::getMetadataPath() const {
  // Store metadata in the same directory as session config
  fs::path sessionPath(sessionConfigPath_);
  return (sessionPath.parent_path() / "conf_metadata.json").string();
}

std::string ConfigSession::getServiceName(cli::ServiceType service) {
  // TODO: Add support for multi_switch mode with sw_agent and hw_agent
  switch (service) {
    case cli::ServiceType::AGENT:
      return "wedge_agent";
  }
  throw std::runtime_error("Unknown service type");
}

void ConfigSession::loadMetadata() {
  std::string metadataPath = getMetadataPath();
  // Note: We don't initialize requiredActions_ here since getRequiredAction()
  // returns HITLESS by default for agents not in the map, and
  // updateRequiredAction() handles adding new agents.

  if (!fs::exists(metadataPath)) {
    return;
  }

  std::string content;
  if (!folly::readFile(metadataPath.c_str(), content)) {
    // If we can't read the file, keep defaults
    return;
  }

  // Parse JSON with symbolic enum names using fbthrift's folly_dynamic API
  // LENIENT adherence allows parsing both string names and integer values
  try {
    folly::dynamic json = folly::parseJson(content);
    cli::ConfigSessionMetadata metadata;
    facebook::thrift::from_dynamic(
        metadata,
        json,
        facebook::thrift::dynamic_format::PORTABLE,
        facebook::thrift::format_adherence::LENIENT);
    requiredActions_ = *metadata.action();
    commands_ = *metadata.commands();
  } catch (const std::exception& ex) {
    // If JSON parsing fails, keep defaults
    LOG(WARNING) << "Failed to parse metadata file: " << ex.what();
  }
}

void ConfigSession::saveMetadata() {
  std::string metadataPath = getMetadataPath();

  // Build Thrift metadata struct and serialize to JSON with symbolic enum names
  // Using PORTABLE format for human-readable enum names instead of integers
  cli::ConfigSessionMetadata metadata;
  metadata.action() = requiredActions_;
  metadata.commands() = commands_;

  folly::dynamic json = facebook::thrift::to_dynamic(
      metadata, facebook::thrift::dynamic_format::PORTABLE);
  std::string prettyJson = folly::toPrettyJson(json);
  folly::writeFileAtomic(
      metadataPath, prettyJson, 0644, folly::SyncType::WITH_SYNC);
}

void ConfigSession::updateRequiredAction(
    cli::ServiceType service,
    cli::ConfigActionLevel actionLevel) {
  // Initialize to HITLESS if not present
  if (requiredActions_.find(service) == requiredActions_.end()) {
    requiredActions_[service] = cli::ConfigActionLevel::HITLESS;
  }
  // Only update if the new action level is higher (more impactful)
  if (static_cast<int>(actionLevel) >
      static_cast<int>(requiredActions_[service])) {
    requiredActions_[service] = actionLevel;
  }
}

cli::ConfigActionLevel ConfigSession::getRequiredAction(
    cli::ServiceType service) const {
  auto it = requiredActions_.find(service);
  if (it != requiredActions_.end()) {
    return it->second;
  }
  return cli::ConfigActionLevel::HITLESS;
}

void ConfigSession::resetRequiredAction(cli::ServiceType service) {
  requiredActions_[service] = cli::ConfigActionLevel::HITLESS;
  commands_.clear();

  // If all services are HITLESS, remove the file entirely
  bool allHitless = true;
  for (const auto& [svc, level] : requiredActions_) {
    if (level != cli::ConfigActionLevel::HITLESS) {
      allHitless = false;
      break;
    }
  }
  if (allHitless) {
    std::string metadataPath = getMetadataPath();
    std::error_code ec;
    fs::remove(metadataPath, ec);
    // Ignore errors - file might not exist
  } else {
    // Only save if there are remaining services with non-HITLESS levels
    saveMetadata();
  }
}

const std::vector<std::string>& ConfigSession::getCommands() const {
  return commands_;
}

void ConfigSession::restartService(
    cli::ServiceType service,
    cli::ConfigActionLevel level) {
  std::string serviceName = getServiceName(service);
  std::string restartType = (level == cli::ConfigActionLevel::AGENT_COLDBOOT)
      ? "coldboot"
      : "warmboot";

  LOG(INFO) << "Restarting " << serviceName << " via systemd (" << restartType
            << ")...";

  // For coldboot, we need to stop the service, create cold_boot_once files,
  // then start the service
  if (level == cli::ConfigActionLevel::AGENT_COLDBOOT) {
    // Step 1: Stop the service
    try {
      folly::Subprocess stopProc(
          {"/usr/bin/sudo", "/usr/bin/systemctl", "stop", serviceName});
      stopProc.waitChecked();
    } catch (const std::exception& ex) {
      throw std::runtime_error(
          fmt::format("Failed to stop {}: {}", serviceName, ex.what()));
    }

    // Step 2: Create coldboot files
    // TODO: Add support for multi_switch mode with hw_agent@0, hw_agent@1, etc.
    const std::vector<std::string> coldbootFiles = {
        "/dev/shm/fboss/warm_boot/cold_boot_once", // for sw_agent
    };
    for (const auto& file : coldbootFiles) {
      // Ensure parent directory exists
      fs::path filePath(file);
      std::error_code ec;
      fs::create_directories(filePath.parent_path(), ec);
      // Create the file (touch equivalent)
      std::ofstream touchFile(file);
      touchFile.close();
      if (!fs::exists(file)) {
        throw std::runtime_error(
            fmt::format("Failed to create coldboot file: {}", file));
      }
    }

    // Step 3: Start the service
    try {
      folly::Subprocess startProc(
          {"/usr/bin/sudo", "/usr/bin/systemctl", "start", serviceName});
      startProc.waitChecked();
    } catch (const std::exception& ex) {
      throw std::runtime_error(
          fmt::format("Failed to start {}: {}", serviceName, ex.what()));
    }
  } else {
    // For warmboot, just do a simple restart
    try {
      folly::Subprocess restartProc(
          {"/usr/bin/sudo", "/usr/bin/systemctl", "restart", serviceName});
      restartProc.waitChecked();
    } catch (const std::exception& ex) {
      throw std::runtime_error(
          fmt::format("Failed to restart {}: {}", serviceName, ex.what()));
    }
  }

  // Wait for the service to be active (up to 60 seconds)
  constexpr int maxWaitSeconds = 60;
  constexpr int pollIntervalMs = 500;
  int waitedMs = 0;

  while (waitedMs < maxWaitSeconds * 1000) {
    try {
      folly::Subprocess checkProc(
          {"/usr/bin/systemctl", "is-active", "--quiet", serviceName});
      checkProc.waitChecked();
      // If waitChecked() doesn't throw, the service is active
      LOG(INFO) << serviceName << " is now active";
      return;
    } catch (const folly::CalledProcessError&) {
      // Service not active yet, keep waiting
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
    waitedMs += pollIntervalMs;
  }

  throw std::runtime_error(
      fmt::format(
          "{} did not become active within {} seconds",
          serviceName,
          maxWaitSeconds));
}

void ConfigSession::reloadServiceConfig(
    cli::ServiceType service,
    const HostInfo& hostInfo) {
  switch (service) {
    case cli::ServiceType::AGENT: {
      auto client = utils::createClient<
          apache::thrift::Client<facebook::fboss::FbossCtrl>>(hostInfo);
      client->sync_reloadConfig();
      LOG(INFO) << "Config reloaded for " << getServiceName(service);
      break;
    }
      // TODO: Add cases for future services (e.g., BGP)
  }
}

void ConfigSession::applyServiceActions(
    const std::map<cli::ServiceType, cli::ConfigActionLevel>& actions,
    const HostInfo& hostInfo) {
  for (const auto& [service, level] : actions) {
    switch (level) {
      case cli::ConfigActionLevel::AGENT_COLDBOOT:
      case cli::ConfigActionLevel::AGENT_WARMBOOT:
        restartService(service, level);
        break;
      case cli::ConfigActionLevel::HITLESS:
        reloadServiceConfig(service, hostInfo);
        break;
    }
  }
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
    // Create initial empty metadata file for new sessions
    saveMetadata();
  } else {
    // Load metadata from disk (survives across CLI invocations)
    loadMetadata();
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

ConfigSession::CommitResult ConfigSession::commit(const HostInfo& hostInfo) {
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

  // Copy the metadata file alongside the config revision
  // e.g., agent-r123.conf -> agent-r123.metadata.json
  // This is required for rollback functionality
  std::string metadataPath = getMetadataPath();
  std::string targetMetadataPath =
      fmt::format("{}/agent-r{}.metadata.json", cliConfigDir_, revision);
  fs::copy_file(metadataPath, targetMetadataPath, ec);
  if (ec) {
    // Clean up the revision file we created
    fs::remove(targetConfigPath);
    throw std::runtime_error(
        fmt::format(
            "Failed to copy metadata to {}: {}",
            targetMetadataPath,
            ec.message()));
  }

  // Atomically update the symlink to point to the new config
  atomicSymlinkUpdate(systemConfigPath_, targetConfigPath);

  // Apply the config based on the required action levels for each service
  // Copy requiredActions_ before we reset it - this will be returned in
  // CommitResult
  auto actions = requiredActions_;

  try {
    applyServiceActions(actions, hostInfo);
    LOG(INFO) << "Config committed as revision r" << revision;
  } catch (const std::exception& ex) {
    // Rollback: atomically restore the old symlink, then re-apply actions
    // on the old config so services pick up the previous configuration
    try {
      atomicSymlinkUpdate(systemConfigPath_, oldSymlinkTarget);
      applyServiceActions(actions, hostInfo);
    } catch (const std::exception& rollbackEx) {
      // If rollback also fails, include both errors in the message
      throw std::runtime_error(
          fmt::format(
              "Failed to apply config: {}. Additionally, failed to rollback the config: {}",
              ex.what(),
              rollbackEx.what()));
    }
    throw std::runtime_error(
        fmt::format(
            "Failed to apply config, config was rolled back automatically: {}",
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

  // Reset action level for all services after successful commit
  for (const auto& [service, level] : actions) {
    resetRequiredAction(service);
  }

  return CommitResult{revision, actions};
}

int ConfigSession::rollback(const HostInfo& hostInfo) {
  // Get the current revision number
  int currentRevision = getCurrentRevisionNumber(systemConfigPath_);
  if (currentRevision <= 0) {
    throw std::runtime_error(
        "Cannot rollback: cannot determine the current revision from " +
        systemConfigPath_);
  } else if (currentRevision == 1) {
    throw std::runtime_error(
        "Cannot rollback: already at the first revision (r1)");
  }

  // Rollback to the previous revision
  std::string targetRevision = "r" + std::to_string(currentRevision - 1);
  return rollback(hostInfo, targetRevision);
}

int ConfigSession::rollback(
    const HostInfo& hostInfo,
    const std::string& revision) {
  ensureDirectoryExists(cliConfigDir_);

  // Build the path to the target revision
  std::string targetConfigPath =
      fmt::format("{}/agent-{}.conf", cliConfigDir_, revision);

  // Check if the target revision exists
  if (!fs::exists(targetConfigPath)) {
    throw std::runtime_error(
        fmt::format(
            "Revision {} does not exist at {}", revision, targetConfigPath));
  }

  std::error_code ec;

  // Verify that the system config is a symlink
  if (!fs::is_symlink(systemConfigPath_)) {
    throw std::runtime_error(
        fmt::format(
            "{} is not a symlink. Expected it to be a symlink.",
            systemConfigPath_));
  }

  // Read the old symlink target in case we need to undo the rollback
  std::string oldSymlinkTarget = fs::read_symlink(systemConfigPath_, ec);
  if (ec) {
    throw std::runtime_error(
        fmt::format(
            "Failed to read symlink {}: {}", systemConfigPath_, ec.message()));
  }

  // First, create a new revision with the same content as the target revision
  auto [newRevisionPath, newRevision] =
      createNextRevisionFile(fmt::format("{}/agent", cliConfigDir_));

  // Copy the target config to the new revision file
  fs::copy_file(
      targetConfigPath,
      newRevisionPath,
      fs::copy_options::overwrite_existing,
      ec);
  if (ec) {
    // Clean up the revision file we created
    fs::remove(newRevisionPath);
    throw std::runtime_error(
        fmt::format(
            "Failed to create new revision for rollback: {}", ec.message()));
  }

  // Atomically update the symlink to point to the new revision
  atomicSymlinkUpdate(systemConfigPath_, newRevisionPath);

  // Reload the config - if this fails, atomically undo the rollback
  // TODO: look at all the metadata files in the revision range and
  // decide whether or not we need to restart the agent based on the highest
  // action level in that range.
  try {
    auto client =
        utils::createClient<apache::thrift::Client<facebook::fboss::FbossCtrl>>(
            hostInfo);
    client->sync_reloadConfig();
  } catch (const std::exception& ex) {
    // Rollback: atomically restore the old symlink
    try {
      atomicSymlinkUpdate(systemConfigPath_, oldSymlinkTarget);
    } catch (const std::exception& rollbackEx) {
      // If rollback also fails, include both errors in the message
      throw std::runtime_error(
          fmt::format(
              "Failed to reload config: {}. Additionally, failed to rollback the symlink: {}",
              ex.what(),
              rollbackEx.what()));
    }
    throw std::runtime_error(
        fmt::format(
            "Failed to reload config, symlink was rolled back automatically: {}",
            ex.what()));
  }

  // Successfully rolled back
  LOG(INFO) << "Rollback committed as revision r" << newRevision;
  return newRevision;
}

} // namespace facebook::fboss
