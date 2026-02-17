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

#include <fmt/format.h>
#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/Subprocess.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <glog/logging.h>
#include <pwd.h>
#include <sys/types.h>
#include <thrift/lib/cpp2/folly_dynamic/folly_dynamic.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>
#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/Git.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
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
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
  std::string tmpLinkName = fmt::format("fboss2_tmp_{}_{}", getpid(), ns);
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
 * Read the command line from /proc/self/cmdline, skipping argv[0].
 * Returns the command arguments as a space-separated string,
 * e.g., "config interface eth1/1/1 mtu 9000"
 */
std::string readCommandLineFromProc() {
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

// Maximum number of conflicts to report before truncating with "and more"
constexpr size_t kMaxConflicts = 10;

// Add a conflict to the list, appending "and more" when we hit the limit.
void addConflict(std::vector<std::string>& conflicts, std::string conflict) {
  conflicts.push_back(std::move(conflict));
  if (conflicts.size() == kMaxConflicts - 1) {
    conflicts.emplace_back("and more");
  }
}

/*
 * Perform a recursive 3-way merge of JSON objects.
 *
 * @param base The original/base version
 * @param head The version that was changed by someone else (current HEAD)
 * @param session The version with the user's changes
 * @param path Current path in the JSON tree (for conflict reporting)
 * @param conflicts Vector to collect conflict paths (capped at kMaxConflicts)
 * @return The merged JSON, preferring session changes over head when safe
 *         (the return value must be ignored when "conflicts" is not empty)
 */
folly::dynamic threeWayMerge(
    const folly::dynamic& base,
    const folly::dynamic& head,
    const folly::dynamic& session,
    const std::string& path,
    std::vector<std::string>& conflicts) {
  // If we've already hit max conflicts, stop recursing
  if (conflicts.size() >= kMaxConflicts) {
    return session;
  }

  // Note: folly::dynamic::operator== does deep comparison which is O(n) for the
  // entire subtree. We compare subtrees O(n) times leading to O(n²) complexity.
  // While suboptimal, benchmarking showed ~11ms for a 41k line config file,
  // which is acceptable for CLI usage, given how simple the implementation is.

  // If session equals base, user didn't change this - use head's version
  if (session == base) {
    return head;
  }

  // If head equals base, other user didn't change this - use session's version
  if (head == base) {
    return session;
  }

  // Both changed - if they made the same change, that's fine
  if (head == session) {
    return session;
  }

  // Both changed differently - need to handle based on type
  if (base.isObject() && head.isObject() && session.isObject()) {
    // Recursively merge objects
    folly::dynamic result = folly::dynamic::object;

    // Collect all keys from all three versions
    std::set<std::string> allKeys;
    for (const auto& kv : base.items()) {
      allKeys.insert(kv.first.asString());
    }
    for (const auto& kv : head.items()) {
      allKeys.insert(kv.first.asString());
    }
    for (const auto& kv : session.items()) {
      allKeys.insert(kv.first.asString());
    }

    for (const auto& key : allKeys) {
      std::string childPath =
          path.empty() ? key : fmt::format("{}.{}", path, key);

      // Get values from each version (null if not present)
      folly::dynamic baseVal = base.getDefault(key, nullptr);
      folly::dynamic headVal = head.getDefault(key, nullptr);
      folly::dynamic sessionVal = session.getDefault(key, nullptr);

      folly::dynamic mergedVal =
          threeWayMerge(baseVal, headVal, sessionVal, childPath, conflicts);

      // Don't include null values (represents deletion)
      if (!mergedVal.isNull()) {
        result[key] = std::move(mergedVal);
      }
    }
    return result;
  }

  if (base.isArray() && head.isArray() && session.isArray()) {
    // For arrays, we can try element-by-element merge if sizes match
    if (base.size() == head.size() && base.size() == session.size()) {
      folly::dynamic result = folly::dynamic::array;
      for (size_t i = 0; i < base.size(); ++i) {
        std::string childPath = fmt::format("{}[{}]", path, i);
        result.push_back(
            threeWayMerge(base[i], head[i], session[i], childPath, conflicts));
      }
      return result;
    }
    // Array sizes differ - this is a conflict
    addConflict(conflicts, path + " (array size mismatch)");
    return session; // Return session's version, but report conflict
  }

  // Scalar values that both changed differently - conflict
  addConflict(conflicts, path);
  return session; // Return session's version, but report conflict
}

} // anonymous namespace

ConfigSession::ConfigSession()
    : sessionConfigDir_(getHomeDirectory() + "/.fboss2"),
      username_(getUsername()) {
  // Use AgentDirectoryUtil to get the config directory path
  // getConfigDirectory() returns /etc/coop/agent, so we get the parent to get
  // /etc/coop
  AgentDirectoryUtil dirUtil;
  std::string coopDir =
      fs::path(dirUtil.getConfigDirectory()).parent_path().string();

  systemConfigDir_ = coopDir;
  git_ = std::make_unique<Git>(coopDir);
  initializeSession();
}

ConfigSession::ConfigSession(
    std::string sessionConfigDir,
    std::string systemConfigDir)
    : sessionConfigDir_(std::move(sessionConfigDir)),
      systemConfigDir_(std::move(systemConfigDir)),
      username_(getUsername()),
      git_(std::make_unique<Git>(systemConfigDir_)) {
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

// Static path getters - can be called without creating a session instance
std::string ConfigSession::getSessionDir() {
  return getHomeDirectory() + "/.fboss2";
}

std::string ConfigSession::getSessionConfigPathStatic() {
  return getSessionDir() + "/agent.conf";
}

std::string ConfigSession::getSessionMetadataPathStatic() {
  return getSessionDir() + "/cli_metadata.json";
}

std::string ConfigSession::getSessionConfigPath() const {
  return sessionConfigDir_ + "/agent.conf";
}

std::string ConfigSession::getSystemConfigPath() const {
  return systemConfigDir_ + "/agent.conf";
}

std::string ConfigSession::getCliConfigDir() const {
  return systemConfigDir_ + "/cli";
}

std::string ConfigSession::getCliConfigPath() const {
  return systemConfigDir_ + "/cli/agent.conf";
}

bool ConfigSession::sessionExists() const {
  return fs::exists(getSessionConfigPath());
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
      getSessionConfigPath(), prettyJson, 0644, folly::SyncType::WITH_SYNC);

  // Update the required action metadata for this service
  updateRequiredAction(service, actionLevel);
}

Git& ConfigSession::getGit() {
  return *git_;
}

const Git& ConfigSession::getGit() const {
  return *git_;
}

std::string ConfigSession::getMetadataPath() const {
  // Store metadata in the same directory as session config
  return sessionConfigDir_ + "/cli_metadata.json";
}

std::string ConfigSession::getSystemMetadataPath() const {
  // Store system metadata in the CLI config directory (Git-versioned)
  return getCliConfigDir() + "/cli_metadata.json";
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
    base_ = *metadata.base();
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
  metadata.base() = base_;

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
    saveActionLevel();
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
    saveActionLevel();
  }
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
      folly::Subprocess stopProc({"/usr/bin/systemctl", "stop", serviceName});
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
      folly::Subprocess startProc({"/usr/bin/systemctl", "start", serviceName});
      startProc.waitChecked();
    } catch (const std::exception& ex) {
      throw std::runtime_error(
          fmt::format("Failed to start {}: {}", serviceName, ex.what()));
    }
  } else {
    // For warmboot, just do a simple restart
    try {
      folly::Subprocess restartProc(
          {"/usr/bin/systemctl", "restart", serviceName});
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
  // If session file doesn't exist (e.g., after a commit), re-initialize
  // the session by copying from system config.
  if (!sessionExists()) {
    initializeSession();
  }

  std::string configJson;
  std::string sessionConfigPath = getSessionConfigPath();
  if (!folly::readFile(sessionConfigPath.c_str(), configJson)) {
    throw std::runtime_error(
        fmt::format("Failed to read config file: {}", sessionConfigPath));
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
  initializeGit();
  if (!sessionExists()) {
    // Starting a new session - reset all state to ensure we don't carry over
    // stale data from a previous session (e.g., if the singleton persisted
    // in memory but the session files were deleted).
    commands_.clear();
    requiredActions_.clear();
    configLoaded_ = false;

    // Ensure the session config directory exists
    ensureDirectoryExists(sessionConfigDir_);
    copySystemConfigToSession();
    // Capture the current git HEAD as the base for this session.
    // This is used to detect if someone else committed changes while this
    // session was in progress.
    base_ = git_->getHead();
    // Create initial metadata file for new sessions
    saveMetadata();
  } else {
    // Load metadata from disk (survives across CLI invocations)
    loadMetadata();
  }
}

void ConfigSession::initializeGit() {
  // Initialize Git repository if it doesn't exist
  if (!git_->isRepository()) {
    ensureDirectoryExists(getCliConfigDir());
    git_->init();
  }

  // If the repository has no commits but the config file exists,
  // create an initial commit to track the existing configuration
  std::string cliConfigPath = getCliConfigPath();
  if (!git_->hasCommits() && fs::exists(cliConfigPath)) {
    std::string systemConfigPath = getSystemConfigPath();
    git_->commit(
        {cliConfigPath, systemConfigPath}, "Initial commit", username_, "");
  }
}

void ConfigSession::copySystemConfigToSession() const {
  // Read system config and write atomically to session config
  // This ensures that readers never see a partially written file - they either
  // see the old file or the new file, never a mix.
  // WITH_SYNC ensures data is flushed to disk before the atomic rename.
  std::string configData;
  std::string systemConfigPath = getSystemConfigPath();
  if (!folly::readFile(systemConfigPath.c_str(), configData)) {
    throw std::runtime_error(
        fmt::format("Failed to read config from {}", systemConfigPath));
  }

  folly::writeFileAtomic(
      getSessionConfigPath(), configData, 0644, folly::SyncType::WITH_SYNC);
}

ConfigSession::CommitResult ConfigSession::commit(const HostInfo& hostInfo) {
  if (!sessionExists()) {
    throw std::runtime_error(
        "No config session exists. Make a config change first.");
  }

  // Check if someone else committed changes while this session was in progress
  std::string currentHead = git_->getHead();
  if (!base_.empty() && currentHead != base_) {
    throw std::runtime_error(
        fmt::format(
            "Cannot commit: the system configuration has changed since this "
            "session was started. Your session was based on commit {}, but the "
            "current HEAD is {}. Run 'config session rebase' to rebase your "
            "changes onto the current configuration, or discard your session "
            "and start over.",
            base_.substr(0, 7),
            currentHead.substr(0, 7)));
  }

  std::string cliConfigDir = getCliConfigDir();
  std::string cliConfigPath = getCliConfigPath();
  std::string sessionConfigPath = getSessionConfigPath();
  std::string systemConfigPath = getSystemConfigPath();

  ensureDirectoryExists(cliConfigDir);

  // Read the session config content
  std::string sessionConfigData;
  if (!folly::readFile(sessionConfigPath.c_str(), sessionConfigData)) {
    throw std::runtime_error(
        fmt::format(
            "Failed to read session config from {}", sessionConfigPath));
  }

  // Read the old config for rollback if needed
  std::string oldConfigData;
  if (fs::exists(cliConfigPath)) {
    if (!folly::readFile(cliConfigPath.c_str(), oldConfigData)) {
      throw std::runtime_error(
          fmt::format("Failed to read CLI config from {}", cliConfigPath));
    }
  }

  // Copy the metadata file alongside the config revision
  // This is required for rollback functionality
  std::string metadataPath = getMetadataPath();
  std::string targetMetadataPath =
      fmt::format("{}/cli_metadata.json", cliConfigDir);
  std::error_code ec;
  fs::copy_file(
      metadataPath,
      targetMetadataPath,
      fs::copy_options::overwrite_existing,
      ec);
  if (ec) {
    if (!oldConfigData.empty()) {
      folly::writeFileAtomic(
          cliConfigPath, oldConfigData, 0644, folly::SyncType::WITH_SYNC);
    }
    throw std::runtime_error(
        fmt::format(
            "Failed to copy metadata to {}: {}",
            targetMetadataPath,
            ec.message()));
  }

  // Atomically write the session config to the CLI config path
  folly::writeFileAtomic(
      cliConfigPath, sessionConfigData, 0644, folly::SyncType::WITH_SYNC);

  // Ensure the system config symlink points to the CLI config
  atomicSymlinkUpdate(systemConfigPath, "cli/agent.conf");

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
  ec = std::error_code{};
  fs::remove(sessionConfigPath, ec);
  if (ec) {
    // Log warning but don't fail - the commit succeeded
    LOG(WARNING) << fmt::format(
        "Failed to remove session config {}: {}",
        sessionConfigPath,
        ec.message());
  }

  // Reset action level for all services after successful commit
  for (const auto& [service, level] : actions) {
    resetRequiredAction(service);
  }

  return CommitResult{revision, actions};
}

void ConfigSession::rebase() {
  if (!sessionExists()) {
    throw std::runtime_error(
        "No config session exists. Make a config change first.");
  }

  std::string currentHead = git_->getHead();

  // If base is empty or already matches HEAD, nothing to rebase
  if (base_.empty() || base_ == currentHead) {
    throw std::runtime_error(
        "No rebase needed: session is already based on the current HEAD.");
  }

  // Get the three versions of the config:
  // 1. Base config (what the session was originally based on)
  // 2. Current HEAD config (what someone else committed)
  // 3. Session config (user's changes)
  std::string cliConfigRelPath = "cli/agent.conf";
  std::string baseConfig = git_->fileAtRevision(base_, cliConfigRelPath);
  std::string headConfig = git_->fileAtRevision(currentHead, cliConfigRelPath);

  std::string sessionConfigPath = getSessionConfigPath();
  std::string sessionConfig;
  if (!folly::readFile(sessionConfigPath.c_str(), sessionConfig)) {
    throw std::runtime_error(
        fmt::format(
            "Failed to read session config from {}", sessionConfigPath));
  }

  // Parse all three as JSON
  folly::dynamic baseJson = folly::parseJson(baseConfig);
  folly::dynamic headJson = folly::parseJson(headConfig);
  folly::dynamic sessionJson = folly::parseJson(sessionConfig);

  // Perform a 3-way merge
  // For each key in session that differs from base, apply to head
  // If head also changed the same key differently, that's a conflict
  std::vector<std::string> conflicts;
  folly::dynamic mergedJson =
      threeWayMerge(baseJson, headJson, sessionJson, "", conflicts);

  if (!conflicts.empty()) {
    std::string conflictList;
    for (const auto& conflict : conflicts) {
      conflictList += "\n  - " + conflict;
    }
    throw std::runtime_error(
        fmt::format(
            "Rebase failed due to conflicts at the following paths:{}",
            conflictList));
  }

  // Write the merged config to the session file
  std::string mergedConfigStr = folly::toPrettyJson(mergedJson);
  folly::writeFileAtomic(
      sessionConfigPath, mergedConfigStr, 0644, folly::SyncType::WITH_SYNC);

  // Update the base to current HEAD
  base_ = currentHead;
  saveMetadata();

  // Reload the config into memory
  loadConfig();
}

std::string ConfigSession::rollback(const HostInfo& hostInfo) {
  // Get the commit history to find the previous commit
  auto commits = git_->log(getCliConfigPath(), 2);
  if (commits.size() < 2) {
    throw std::runtime_error(
        "Cannot rollback: no previous revision available in Git history");
  }

  // Rollback to the previous commit (second in the list)
  return rollback(hostInfo, commits[1].sha1);
}

std::string ConfigSession::rollback(
    const HostInfo& hostInfo,
    const std::string& commitSha) {
  std::string cliConfigDir = getCliConfigDir();
  std::string cliConfigPath = getCliConfigPath();
  std::string systemConfigPath = getSystemConfigPath();

  ensureDirectoryExists(cliConfigDir);

  // Resolve the commit SHA (in case it's a short SHA or ref)
  std::string resolvedSha = git_->resolveRef(commitSha);

  // Get the config and metadata content from the target commit
  // The paths in git are relative to the repo root
  std::string targetConfigData =
      git_->fileAtRevision(resolvedSha, "cli/agent.conf");
  std::string targetMetadataData =
      git_->fileAtRevision(resolvedSha, "cli/cli_metadata.json");
  std::string metadataPath = fmt::format("{}/cli_metadata.json", cliConfigDir);

  // Read the current config for rollback if needed
  std::string oldConfigData;
  if (fs::exists(cliConfigPath)) {
    if (!folly::readFile(cliConfigPath.c_str(), oldConfigData)) {
      throw std::runtime_error(
          fmt::format("Failed to read current config from {}", cliConfigPath));
    }
  }
  std::string oldMetadataData;
  if (fs::exists(metadataPath)) {
    if (!folly::readFile(metadataPath.c_str(), oldMetadataData)) {
      throw std::runtime_error(
          fmt::format("Failed to read current metadata from {}", metadataPath));
    }
  }

  // Atomically write the target config and metadata to the CLI directory
  folly::writeFileAtomic(
      cliConfigPath, targetConfigData, 0644, folly::SyncType::WITH_SYNC);
  folly::writeFileAtomic(
      metadataPath, targetMetadataData, 0644, folly::SyncType::WITH_SYNC);

  // Ensure the system config symlink points to the CLI config
  atomicSymlinkUpdate(systemConfigPath, "cli/agent.conf");

  // Reload the config - if this fails, restore the old config and metadata
  // TODO: look at all the metadata files in the revision range and
  // decide whether or not we need to restart the agent based on the highest
  // action level in that range.
  std::string newCommitSha;
  try {
    auto client =
        utils::createClient<apache::thrift::Client<facebook::fboss::FbossCtrl>>(
            hostInfo);
    client->sync_reloadConfig();

    // Create a Git commit for the rollback with all relevant files
    std::string commitMessage = fmt::format(
        "Rollback to {} by {}", resolvedSha.substr(0, 8), username_);
    newCommitSha = git_->commit(
        {cliConfigPath, metadataPath, systemConfigPath},
        commitMessage,
        username_,
        "");
    LOG(INFO) << "Rollback committed as " << newCommitSha.substr(0, 8);
  } catch (const std::exception& ex) {
    // Rollback: restore the old config and metadata
    try {
      if (!oldConfigData.empty()) {
        folly::writeFileAtomic(
            cliConfigPath, oldConfigData, 0644, folly::SyncType::WITH_SYNC);
      }
      if (!oldMetadataData.empty()) {
        folly::writeFileAtomic(
            metadataPath, oldMetadataData, 0644, folly::SyncType::WITH_SYNC);
      }
    } catch (const std::exception& rollbackEx) {
      // If rollback also fails, include both errors in the message
      throw std::runtime_error(
          fmt::format(
              "Failed to reload config: {}. Additionally, failed to restore the old config: {}",
              ex.what(),
              rollbackEx.what()));
    }
    throw std::runtime_error(
        fmt::format(
            "Failed to reload config, config was restored automatically: {}",
            ex.what()));
  }

  return newCommitSha;
}

} // namespace facebook::fboss
