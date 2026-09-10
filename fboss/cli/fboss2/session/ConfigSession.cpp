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
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <glog/logging.h>
#include <neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
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
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/FbossServiceUtil.h"
#include "fboss/cli/fboss2/session/Git.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
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

ConfigSession::ConfigSession(SessionInit init) {
  username_ = getUsername();
  std::string homeDir = getHomeDirectory();

  // Use AgentDirectoryUtil to get the config directory path
  // getConfigDirectory() returns /etc/coop/agent, so we get the parent to get
  // /etc/coop
  AgentDirectoryUtil dirUtil;
  std::string coopDir =
      fs::path(dirUtil.getConfigDirectory()).parent_path().string();

  sessionConfigDir_ = homeDir + "/.fboss2";
  systemConfigDir_ = coopDir;
  git_ = std::make_unique<Git>(coopDir);
  initializeSession(init);
}

ConfigSession::ConfigSession(
    std::string sessionConfigDir,
    std::string systemConfigDir,
    SessionInit init)
    : sessionConfigDir_(std::move(sessionConfigDir)),
      systemConfigDir_(std::move(systemConfigDir)),
      username_(getUsername()),
      git_(std::make_unique<Git>(systemConfigDir_)) {
  initializeSession(init);
}

ConfigSession::ConfigSession(
    std::string sessionConfigDir,
    std::string systemConfigDir,
    std::unique_ptr<FbossServiceUtil> fbossServiceUtil)
    : fbossServiceUtil_(std::move(fbossServiceUtil)),
      sessionConfigDir_(std::move(sessionConfigDir)),
      systemConfigDir_(std::move(systemConfigDir)),
      username_(getUsername()),
      git_(std::make_unique<Git>(systemConfigDir_)) {
  // Don't call initializeSession() - this constructor is for testing only
  // and tests don't need git initialization or config file copying
}

// Out-of-line so the unique_ptr members' (forward-declared) types are complete
// here where they are destroyed.
ConfigSession::~ConfigSession() = default;

namespace {
std::unique_ptr<ConfigSession>& getInstancePtr() {
  static std::unique_ptr<ConfigSession> instance;
  return instance;
}
} // namespace

ConfigSession& ConfigSession::getInstance(SessionInit init) {
  auto& instance = getInstancePtr();
  if (!instance) {
    instance = std::make_unique<ConfigSession>(init);
  }
  return *instance;
}

void ConfigSession::setInstance(std::unique_ptr<ConfigSession> newInstance) {
  getInstancePtr() = std::move(newInstance);
}

void ConfigSession::resetInstance() {
  setInstance(nullptr);
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

std::string ConfigSession::getBgpSessionConfigPathStatic() {
  return getSessionDir() + "/bgp_config.json";
}

std::vector<std::string> ConfigSession::stagedSessionFilePaths() {
  // Per-domain staged config files plus the session metadata. Keep this in sync
  // with configDomains() (the sessionPath of each domain); a new domain adds
  // one line here.
  return {
      getSessionConfigPathStatic(), // agent: ~/.fboss2/agent.conf
      getBgpSessionConfigPathStatic(), // bgp:  ~/.fboss2/bgp_config.json
      getSessionMetadataPathStatic(), // shared: ~/.fboss2/cli_metadata.json
  };
}

std::string ConfigSession::fileAtRevisionOrEmpty(
    const std::string& revision,
    const std::string& gitRelPath) const {
  try {
    return git_->fileAtRevision(revision, gitRelPath);
  } catch (const std::exception&) {
    // The path did not exist at that revision (e.g. a commit predating BGP
    // config). Treat as empty.
    return "";
  }
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

std::vector<ConfigSession::ConfigDomain> ConfigSession::configDomains() const {
  return {
      ConfigDomain{
          cli::ServiceType::AGENT,
          "Agent",
          getSessionConfigPath(), // ~/.fboss2/agent.conf
          kAgentGitRelPath, // cli/agent.conf
          getCliConfigPath(), // /etc/coop/cli/agent.conf (promoted)
          getSystemConfigPath(), // /etc/coop/agent.conf (symlink, live read)
          getSystemConfigPath(), // symlink IS the system path for the agent
          kAgentGitRelPath, // symlink -> cli/agent.conf
          // Rollback floor: reload the agent, unless a commit being undone
          // recorded a higher level (see rolledBackActionLevels()).
          cli::ConfigActionLevel::HITLESS,
      },
      ConfigDomain{
          cli::ServiceType::BGP,
          "BGP",
          getBgpSessionConfigPath(), // ~/.fboss2/bgp_config.json
          kBgpGitRelPath, // bgpcpp/bgpcpp.conf
          getBgpSystemConfigPath(), // /etc/coop/bgpcpp/bgpcpp.conf (promoted)
          getBgpSystemConfigLinkPath(), // /etc/coop/bgpcpp.conf (the symlink,
                                        // as for the agent: it is what bgpd
                                        // reads, and before the first commit
                                        // it is the image-installed file)
          getBgpSystemConfigLinkPath(), // /etc/coop/bgpcpp.conf (symlink)
          kBgpGitRelPath, // symlink -> bgpcpp/bgpcpp.conf
          cli::ConfigActionLevel::SERVICE_RESTART, // rollback restarts bgpd
      },
  };
}

std::optional<std::string> ConfigSession::readStagedContent(
    const ConfigDomain& domain) const {
  if (!fs::exists(domain.sessionPath)) {
    return std::nullopt;
  }
  std::string content;
  if (!folly::readFile(domain.sessionPath.c_str(), content)) {
    throw std::runtime_error(
        fmt::format(
            "Failed to read session config from {}", domain.sessionPath));
  }
  return content;
}

std::string ConfigSession::readPromotedContent(
    const ConfigDomain& domain) const {
  std::string content;
  if (fs::exists(domain.promotedPath)) {
    if (!folly::readFile(domain.promotedPath.c_str(), content)) {
      throw std::runtime_error(
          fmt::format(
              "Failed to read current config from {}", domain.promotedPath));
    }
  }
  return content;
}

void ConfigSession::promoteDomain(
    const ConfigDomain& domain,
    const std::string& content,
    std::vector<std::string>& commitFiles) const {
  ensureDirectoryExists(fs::path(domain.promotedPath).parent_path().string());
  folly::writeFileAtomic(
      domain.promotedPath, content, 0644, folly::SyncType::WITH_SYNC);
  commitFiles.push_back(domain.promotedPath);
  // Keep the daemon-facing path a symlink into the CLI-managed dir so the
  // daemon needs no per-device --config override. The symlink is git-tracked
  // alongside the config so a rollback restores it.
  atomicSymlinkUpdate(domain.symlinkPath, domain.symlinkTarget);
  commitFiles.push_back(domain.symlinkPath);
}

void ConfigSession::restorePromotedDomain(
    const ConfigDomain& domain,
    const std::string& oldContent,
    bool existed) const {
  if (existed) {
    folly::writeFileAtomic(
        domain.promotedPath, oldContent, 0644, folly::SyncType::WITH_SYNC);
  } else {
    std::error_code rmEc;
    fs::remove(domain.promotedPath, rmEc);
  }
}

void ConfigSession::clearStagedDomain(const ConfigDomain& domain) {
  std::error_code ec;
  fs::remove(domain.sessionPath, ec);
  if (ec) {
    LOG(WARNING) << fmt::format(
        "Failed to remove session config {}: {}",
        domain.sessionPath,
        ec.message());
  }
  // Drop the in-memory cache (null == not loaded) so the next access re-seeds
  // from the promoted config.
  switch (domain.service) {
    case cli::ServiceType::AGENT:
      agentConfig_.reset();
      break;
    case cli::ServiceType::BGP:
      bgpConfig_.reset();
      break;
  }
}

bool ConfigSession::domainContentEqual(
    const ConfigDomain& domain,
    const std::string& a,
    const std::string& b) const {
  // Empty (missing) content cannot be parsed as a struct; compare bytes. Both
  // empty -> equal; empty vs non-empty -> changed.
  if (a.empty() || b.empty()) {
    return a == b;
  }
  try {
    switch (domain.service) {
      case cli::ServiceType::AGENT: {
        cfg::AgentConfig sa, sb;
        apache::thrift::SimpleJSONSerializer::deserialize<cfg::AgentConfig>(
            a, sa);
        apache::thrift::SimpleJSONSerializer::deserialize<cfg::AgentConfig>(
            b, sb);
        return sa == sb;
      }
      case cli::ServiceType::BGP: {
        bgp::thrift::BgpConfig sa, sb;
        apache::thrift::SimpleJSONSerializer::deserialize<
            bgp::thrift::BgpConfig>(a, sa);
        apache::thrift::SimpleJSONSerializer::deserialize<
            bgp::thrift::BgpConfig>(b, sb);
        return sa == sb;
      }
    }
  } catch (const std::exception& ex) {
    // Malformed JSON on either side: fall back to a byte comparison rather than
    // crashing the commit/rollback. Differing bytes are then treated as a
    // change (the safe, conservative outcome).
    LOG(WARNING) << "Semantic config comparison for " << domain.name
                 << " failed to parse; falling back to byte comparison: "
                 << ex.what();
    return a == b;
  }
  return a == b; // unreachable: switch above is exhaustive
}

bool ConfigSession::sessionExists() const {
  return fs::exists(getSessionConfigPath());
}

bool ConfigSession::hasActiveSession() const {
  // An agent config session (agent.conf) OR a protocol session staged outside
  // agent.conf. BGP is the latter: a staged ~/.fboss2/bgp_config.json (written
  // by either the typed global config here or BgpConfigSession's peer edits)
  // with a recorded restart (SERVICE_RESTART) action, but never touching
  // agent.conf.
  return sessionExists() || bgpSessionExists();
}

cfg::AgentConfig& ConfigSession::getAgentConfig() {
  if (!agentConfig_) {
    loadConfig();
  }
  return *agentConfig_;
}

const cfg::AgentConfig& ConfigSession::getAgentConfig() const {
  if (!agentConfig_) {
    throw std::runtime_error(
        "Config not loaded yet. Call getAgentConfig() (non-const) first.");
  }
  return *agentConfig_;
}

utils::PortMap& ConfigSession::getPortMap() {
  if (!agentConfig_) {
    loadConfig();
  }
  return *portMap_;
}

const utils::PortMap& ConfigSession::getPortMap() const {
  if (!agentConfig_) {
    throw std::runtime_error(
        "Config not loaded yet. Call getPortMap() (non-const) first.");
  }
  return *portMap_;
}

void ConfigSession::rebuildPortMap() {
  if (!agentConfig_) {
    loadConfig();
  }
  portMap_ = std::make_unique<utils::PortMap>(*agentConfig_);
}

void ConfigSession::saveConfig(
    cli::ServiceType service,
    cli::ConfigActionLevel actionLevel) {
  // Serialize whichever typed config this service owns and stage it to that
  // domain's session file. The round-trip through serialize -> parse ->
  // toPrettyJson is needed because SimpleJSONSerializer emits Thrift maps with
  // integer keys (e.g. clientIdToAdminDistance) as string keys; going through
  // facebook::thrift::to_dynamic() directly would keep integer keys and make
  // folly::toPrettyJson() fail (JSON object keys must be strings).
  std::string prettyJson;
  std::string sessionPath;
  switch (service) {
    case cli::ServiceType::AGENT: {
      if (!agentConfig_) {
        throw std::runtime_error("No config loaded to save");
      }
      auto json = apache::thrift::SimpleJSONSerializer::serialize<std::string>(
          *agentConfig_);
      prettyJson = folly::toPrettyJson(folly::parseJson(json));
      sessionPath = getSessionConfigPath();
      break;
    }
    case cli::ServiceType::BGP: {
      if (!bgpConfig_) {
        loadBgpConfig();
      }
      auto json = apache::thrift::SimpleJSONSerializer::serialize<std::string>(
          *bgpConfig_);
      prettyJson = folly::toPrettyJson(folly::parseJson(json));
      sessionPath = getBgpSessionConfigPath();
      break;
    }
  }

  // May not exist yet if this session was constructed ReadOnly.
  ensureDirectoryExists(sessionConfigDir_);

  // Use folly::writeFileAtomic with sync to avoid race conditions when multiple
  // threads/processes write to the same session file. WITH_SYNC ensures data
  // is flushed to disk before the atomic rename, preventing readers from
  // seeing partial/corrupted data.
  folly::writeFileAtomic(
      sessionPath, prettyJson, 0644, folly::SyncType::WITH_SYNC);

  // Record the command from /proc/self/cmdline and bump this service's required
  // action level + metadata. Shared with recordServiceAction() so command
  // tracking and action bookkeeping are identical for every service.
  recordServiceAction(service, actionLevel);
}

void ConfigSession::saveConfig() {
  saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
}

void ConfigSession::recordServiceAction(
    cli::ServiceType service,
    cli::ConfigActionLevel actionLevel) {
  // Record the command from /proc/self/cmdline so it shows up in this
  // session's command history, exactly like saveConfig() does. Config for
  // this service lives in a separate file (e.g. ~/.fboss2/bgp_config.json),
  // so we deliberately do NOT touch the agent config here.
  std::string rawCmd = readCommandLineFromProc();
  auto pos = rawCmd.find("config ");
  if (pos != std::string::npos) {
    std::string cmd = rawCmd.substr(pos);
    if (commands_.empty() || commands_.back() != cmd) {
      commands_.push_back(cmd);
    }
  }

  // Update and persist the required action level for this service.
  updateRequiredAction(service, actionLevel);
  saveMetadata();
}

std::string ConfigSession::getBgpSessionConfigPath() const {
  return sessionConfigDir_ + "/bgp_config.json";
}

std::string ConfigSession::getBgpSystemConfigDir() const {
  return systemConfigDir_ + "/bgpcpp";
}

std::string ConfigSession::getBgpSystemConfigPath() const {
  return getBgpSystemConfigDir() + "/bgpcpp.conf";
}

std::string ConfigSession::getBgpSystemConfigLinkPath() const {
  return systemConfigDir_ + "/bgpcpp.conf";
}

bool ConfigSession::bgpSessionExists() const {
  return fs::exists(getBgpSessionConfigPath());
}

void ConfigSession::loadBgpConfig() {
  if (bgpConfig_) {
    return;
  }

  // Prefer staged edits; otherwise seed from the running bgpd config; else
  // schema defaults. A read failure on a file that exists is logged (not
  // silently treated as "no config") so a permission/IO error doesn't
  // masquerade as a fresh session.
  // The running config is read through the daemon's own --config path, which
  // is a symlink to the promoted file once a commit has happened and the
  // plain file the bgp++ RPM installs before that. Seeding from it (rather
  // than from the promoted path directly) is what keeps a first BGP edit on a
  // freshly imaged box from starting at schema defaults and having the commit
  // discard the running config, leaving bgpd to crash-loop on an unset
  // router_id. The promoted path is a backstop for a missing symlink.
  std::string content;
  std::string sessionPath = getBgpSessionConfigPath();
  std::string linkPath = getBgpSystemConfigLinkPath();
  std::string systemPath = getBgpSystemConfigPath();
  if (fs::exists(sessionPath)) {
    if (!folly::readFile(sessionPath.c_str(), content)) {
      LOG(WARNING) << "Failed to read staged BGP config " << sessionPath
                   << "; starting from defaults";
    }
  } else if (fs::exists(linkPath)) {
    if (!folly::readFile(linkPath.c_str(), content)) {
      LOG(WARNING) << "Failed to read system BGP config " << linkPath
                   << "; starting from defaults";
    }
  } else if (fs::exists(systemPath)) {
    if (!folly::readFile(systemPath.c_str(), content)) {
      LOG(WARNING) << "Failed to read promoted BGP config " << systemPath
                   << "; starting from defaults";
    }
  }

  bgpConfig_ = std::make_unique<bgp::thrift::BgpConfig>();
  if (!content.empty()) {
    try {
      apache::thrift::SimpleJSONSerializer::deserialize<bgp::thrift::BgpConfig>(
          content, *bgpConfig_);
    } catch (const std::exception& ex) {
      LOG(WARNING) << "Failed to parse BGP config, starting from defaults: "
                   << ex.what();
      *bgpConfig_ = bgp::thrift::BgpConfig();
    }
  }
}

bgp::thrift::BgpConfig& ConfigSession::getBgpConfig() {
  if (!bgpConfig_) {
    loadBgpConfig();
  }
  return *bgpConfig_;
}

const bgp::thrift::BgpConfig& ConfigSession::getBgpConfig() const {
  if (!bgpConfig_) {
    throw std::runtime_error(
        "BGP config not loaded yet. Call getBgpConfig() (non-const) first.");
  }
  return *bgpConfig_;
}

void ConfigSession::saveBgpConfig() {
  // Convenience wrapper over the generic saveConfig(), mirroring the no-arg
  // saveConfig() for the agent. bgpd has no hitless reload, so a staged BGP
  // change always requires a bgpd restart (SERVICE_RESTART) on the next
  // `config session commit`.
  saveConfig(cli::ServiceType::BGP, cli::ConfigActionLevel::SERVICE_RESTART);
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
  // May not exist yet if this session was constructed ReadOnly.
  ensureDirectoryExists(sessionConfigDir_);

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

std::string ConfigSession::getServiceName(cli::ServiceType service) {
  return FbossServiceUtil::getServiceName(service);
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

void ConfigSession::ensureFbossServiceUtil(const HostInfo& hostInfo) {
  if (!fbossServiceUtil_) {
    MultiSwitchRunState runState;
    try {
      runState = utils::getMultiSwitchRunState(hostInfo);
    } catch (const std::exception& e) {
      throw std::runtime_error(
          fmt::format(
              "Failed to query agent run state from {}. "
              "Is the agent running? Error: {}",
              hostInfo.getName(),
              e.what()));
    }

    std::vector<int> switchIndexes;
    for (const auto& [idx, _] : *runState.hwIndexToRunState()) {
      switchIndexes.push_back(idx);
    }
    std::sort(switchIndexes.begin(), switchIndexes.end());

    fbossServiceUtil_ = std::make_unique<FbossServiceUtil>(
        std::move(switchIndexes), *runState.multiSwitchEnabled());
  }
}

std::map<cli::ServiceType, std::vector<std::string>>
ConfigSession::applyServiceActions(
    const std::map<cli::ServiceType, cli::ConfigActionLevel>& actions,
    const HostInfo& hostInfo) {
  ensureFbossServiceUtil(hostInfo);
  std::map<cli::ServiceType, std::vector<std::string>> serviceNames;
  for (const auto& [service, level] : actions) {
    switch (level) {
      case cli::ConfigActionLevel::DISRUPTIVE_SERVICE_RESTART:
      case cli::ConfigActionLevel::SERVICE_RESTART:
        serviceNames[service] =
            fbossServiceUtil_->restartService(service, level);
        break;
      case cli::ConfigActionLevel::HITLESS:
        serviceNames[service] =
            fbossServiceUtil_->reloadConfig(service, hostInfo);
        break;
    }
  }
  return serviceNames;
}

void ConfigSession::loadConfig() {
  // If session file doesn't exist (e.g., after a commit), re-initialize
  // the session by copying from system config.
  if (!sessionExists()) {
    // Force materialization even if constructed ReadOnly.
    initializeSession(SessionInit::CreateIfAbsent);
  }

  std::string configJson;
  std::string sessionConfigPath = getSessionConfigPath();
  if (!folly::readFile(sessionConfigPath.c_str(), configJson)) {
    throw std::runtime_error(
        fmt::format("Failed to read config file: {}", sessionConfigPath));
  }

  agentConfig_ = std::make_unique<cfg::AgentConfig>();
  apache::thrift::SimpleJSONSerializer::deserialize<cfg::AgentConfig>(
      configJson, *agentConfig_);

  // Handle the legacy case where config might be a bare SwitchConfig
  if (*agentConfig_->sw() == cfg::SwitchConfig()) {
    apache::thrift::SimpleJSONSerializer::deserialize<cfg::SwitchConfig>(
        configJson, *agentConfig_->sw());
  }
  portMap_ = std::make_unique<utils::PortMap>(*agentConfig_);
}

void ConfigSession::initializeSession(SessionInit init) {
  // Bootstraps /etc/coop, not ~/.fboss2, so this runs regardless of `init`.
  initializeGit();
  // Resume an existing session if EITHER an agent (agent.conf) or a BGP
  // (bgp_config.json) session is staged. Keying only on the agent session file
  // would misdetect a BGP-only session as fresh and clear its recorded
  // restart (SERVICE_RESTART) action on the next (separate-process) CLI
  // invocation, silently dropping the staged change at commit time.
  if (!hasActiveSession()) {
    // Starting a new session - reset all state to ensure we don't carry over
    // stale data from a previous session (e.g., if the singleton persisted
    // in memory but the session files were deleted).
    commands_.clear();
    requiredActions_.clear();
    agentConfig_.reset();

    if (init == SessionInit::ReadOnly) {
      return; // leave ~/.fboss2 alone
    }

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

  // Always ensure an initial commit exists so base_ is never empty.
  // Replicates the exact structure that commit() produces:
  //   cli/agent.conf  — the actual config content
  //   agent.conf      — a symlink to cli/agent.conf
  // This prevents two concurrent users from both seeing base_="" and silently
  // overwriting each other's commits, and ensures rebase() can always call
  // git show <base>:cli/agent.conf without hitting exit 128.
  if (!git_->hasCommits()) {
    std::string systemConfigPath = getSystemConfigPath();
    std::string cliConfigPath = getCliConfigPath();

    ensureDirectoryExists(getCliConfigDir());

    // Populate cli/agent.conf if it doesn't exist yet.
    // If agent.conf is currently a plain file (pre-symlink state), copy from
    // it so we don't lose the running config.  We copy rather than rename
    // because /etc/coop/agent.conf must remain in place (the running system
    // reads it, and commit() will later replace it with a symlink).
    // Fall back to an empty JSON object so the file is always valid JSON.
    if (!fs::exists(cliConfigPath)) {
      std::error_code ec;
      bool systemIsRegularFile =
          fs::is_regular_file(fs::path(systemConfigPath), ec) && !ec;
      std::string seedContent = "{}";
      if (systemIsRegularFile) {
        folly::readFile(systemConfigPath.c_str(), seedContent);
        if (seedContent.empty()) {
          seedContent = "{}";
        }
      }
      folly::writeFileAtomic(
          cliConfigPath, seedContent, 0644, folly::SyncType::WITH_SYNC);
    }

    // Seed an empty metadata file and include it in the initial commit so the
    // baseline shows up in `git log -- cli/cli_metadata.json`. No-arg
    // rollback() walks metadata history (so BGP-only commits, which never touch
    // agent.conf, are reachable); without the baseline in that history, rolling
    // back to the very first commit would fail with "no previous revision".
    std::string initialMetadataPath = getSystemMetadataPath();
    if (!fs::exists(initialMetadataPath)) {
      folly::writeFileAtomic(
          initialMetadataPath, "{}", 0644, folly::SyncType::WITH_SYNC);
    }

    // Seed the running bgpd config into the baseline commit too. Without it,
    // the first revision has no BGP snapshot, so a rollback to it would read
    // the empty target as "BGP never existed" and DELETE the running
    // bgpcpp.conf.
    //
    // Mirroring the agent above: if the promoted path doesn't exist yet but
    // the daemon's config path resolves to a readable file (the bgp++ RPM
    // ships it as a plain file), populate the promoted path from it. Copy
    // rather than rename — bgpd reads /etc/coop/bgpcpp.conf right now, and
    // commit() is what later replaces it with a symlink to the promoted copy.
    std::vector<std::string> initialFiles = {
        cliConfigPath, initialMetadataPath};
    std::string bgpSystemPath = getBgpSystemConfigPath();
    std::string bgpLinkPath = getBgpSystemConfigLinkPath();
    if (!fs::exists(bgpSystemPath) && fs::exists(bgpLinkPath)) {
      // fs::exists follows symlinks, so a dangling one is correctly skipped.
      std::string bgpSeedContent;
      if (folly::readFile(bgpLinkPath.c_str(), bgpSeedContent) &&
          !bgpSeedContent.empty()) {
        ensureDirectoryExists(getBgpSystemConfigDir());
        folly::writeFileAtomic(
            bgpSystemPath, bgpSeedContent, 0644, folly::SyncType::WITH_SYNC);
      }
    }
    if (fs::exists(bgpSystemPath)) {
      initialFiles.push_back(bgpSystemPath);
    }

    try {
      git_->commit(initialFiles, "Initial commit", username_, "");
    } catch (const std::exception&) {
      // Another process may have raced us to the initial commit.
      // If commits now exist, swallow the error; otherwise re-throw.
      if (!git_->hasCommits()) {
        throw;
      }
    }
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
  if (!hasActiveSession()) {
    throw std::runtime_error(
        "No config session exists. Make a config change first.");
  }

  // A BGP-only session never ran initializeSession(), so ensure the /etc/coop
  // git repo and its initial commit exist before we commit into it. Idempotent
  // for agent sessions, which initialized git when the session started.
  initializeGit();

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
            Git::shortSha1(base_),
            Git::shortSha1(currentHead)));
  }

  ensureDirectoryExists(getCliConfigDir());

  // Per-domain staged/changed analysis, applied uniformly to agent and BGP.
  // A domain is "staged" when a session edit exists; it is "pending" (needs
  // promotion + a service action) only when the staged content differs from
  // what is already promoted. This skip-when-unchanged rule is the same for
  // both domains, so re-committing an unchanged config is a true no-op: no git
  // revision, no symlink churn, and no reloadConfig()/bgpd restart.
  struct Pending {
    ConfigDomain domain;
    std::string staged;
    std::string oldPromoted;
    bool promotedExisted;
  };
  std::vector<ConfigDomain> stagedDomains;
  std::vector<Pending> pending;
  // actions ends up holding exactly the pending domains' required action levels
  // (returned in CommitResult and passed to applyServiceActions).
  auto actions = requiredActions_;
  for (const auto& domain : configDomains()) {
    auto staged = readStagedContent(domain);
    if (!staged) {
      actions.erase(domain.service); // nothing staged for this domain
      continue;
    }
    stagedDomains.push_back(domain);
    std::string oldPromoted = readPromotedContent(domain);
    if (domainContentEqual(domain, *staged, oldPromoted)) {
      actions.erase(domain.service); // unchanged -> no promote, no action
      continue;
    }
    pending.push_back(
        {domain,
         std::move(*staged),
         std::move(oldPromoted),
         fs::exists(domain.promotedPath)});
  }

  // Nothing that is staged actually changed -> no-op.
  if (pending.empty()) {
    return CommitResult{"", {}, {}};
  }

  // Write the metadata file alongside the config revision (required for
  // rollback). Use folly::writeFileAtomic rather than fs::copy_file so we only
  // write content without fchmod()ing a differently-owned destination (EPERM).
  // Nothing has been promoted yet, so a failure here simply aborts.
  std::string metadataPath = getMetadataPath();
  std::string targetMetadataPath = getSystemMetadataPath();
  std::string metadataContent;
  if (!folly::readFile(metadataPath.c_str(), metadataContent)) {
    LOG(WARNING) << "Failed to read session metadata from " << metadataPath
                 << "; committing empty metadata";
    metadataContent = "{}";
  }
  folly::writeFileAtomic(
      targetMetadataPath, metadataContent, 0664, folly::SyncType::WITH_SYNC);

  std::vector<std::string> commitFiles = {targetMetadataPath};
  std::string commitSha;
  std::map<cli::ServiceType, std::vector<std::string>> serviceNames;

  try {
    // Promote every changed domain (staged -> git-tracked file + daemon
    // symlink) BEFORE applying its service action, so the reload/restart picks
    // up the new config. Session files are left in place until the whole commit
    // succeeds, so a failure here can be rolled back and retried.
    for (const auto& p : pending) {
      promoteDomain(p.domain, p.staged, commitFiles);
    }
    // Track every other domain's running config in this commit too, so a later
    // rollback has a snapshot to restore instead of wiping it (e.g. a
    // bgpcpp.conf present on disk but not yet committed). git dedups unchanged
    // content, so re-adding an already-tracked file is a no-op.
    std::set<cli::ServiceType> pendingServices;
    for (const auto& p : pending) {
      pendingServices.insert(p.domain.service);
    }
    for (const auto& domain : configDomains()) {
      if (pendingServices.count(domain.service) == 0 &&
          fs::exists(domain.promotedPath)) {
        commitFiles.push_back(domain.promotedPath);
      }
    }

    serviceNames = applyServiceActions(actions, hostInfo);

    std::string commitMessage = fmt::format("Config commit by {}", username_);
    commitSha = git_->commit(commitFiles, commitMessage, username_, "");
    LOG(INFO) << "Config committed as " << Git::shortSha1(commitSha);
  } catch (const std::exception& ex) {
    // Restore each promoted domain to its prior state, then re-apply actions on
    // the old config so services pick up the previous configuration. Staged
    // session files are left intact so the user can retry.
    try {
      for (const auto& p : pending) {
        restorePromotedDomain(p.domain, p.oldPromoted, p.promotedExisted);
      }
      applyServiceActions(actions, hostInfo);
    } catch (const std::exception& rollbackEx) {
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

  // The commit fully succeeded: the session is consumed, so clear every staged
  // domain's session file and reset its recorded action level.
  for (const auto& domain : stagedDomains) {
    clearStagedDomain(domain);
    resetRequiredAction(domain.service);
  }
  base_ = commitSha;
  // Force a reload from the promoted config on next access (null == not
  // loaded).
  agentConfig_.reset();
  bgpConfig_.reset();

  return CommitResult{commitSha, actions, serviceNames};
}

void ConfigSession::rebase() {
  if (!hasActiveSession()) {
    throw std::runtime_error(
        "No config session exists. Make a config change first.");
  }

  std::string currentHead = git_->getHead();

  // If base is empty or already matches HEAD, nothing to rebase
  if (base_.empty() || base_ == currentHead) {
    throw std::runtime_error(
        "No rebase needed: session is already based on the current HEAD.");
  }

  std::vector<std::string> conflicts;

  // 3-way merge a single staged file against the base/head revisions of its
  // git-tracked counterpart. Returns the merged pretty JSON, or nullopt if the
  // domain is not staged this session. A missing file at a revision (e.g. a
  // commit predating BGP config) is treated as an empty object.
  auto mergeStaged =
      [&](const std::string& gitRelPath,
          const std::string& sessionPath) -> std::optional<std::string> {
    if (!fs::exists(sessionPath)) {
      return std::nullopt;
    }
    std::string sessionConfig;
    if (!folly::readFile(sessionPath.c_str(), sessionConfig)) {
      throw std::runtime_error(
          fmt::format("Failed to read session config from {}", sessionPath));
    }
    std::string baseConfig = fileAtRevisionOrEmpty(base_, gitRelPath);
    std::string headConfig = fileAtRevisionOrEmpty(currentHead, gitRelPath);
    // A missing file at a revision is treated as an empty object.
    folly::dynamic baseJson = folly::dynamic::object;
    if (!baseConfig.empty()) {
      baseJson = folly::parseJson(baseConfig);
    }
    folly::dynamic headJson = folly::dynamic::object;
    if (!headConfig.empty()) {
      headJson = folly::parseJson(headConfig);
    }
    folly::dynamic sessionJson = folly::parseJson(sessionConfig);
    folly::dynamic merged =
        threeWayMerge(baseJson, headJson, sessionJson, "", conflicts);
    return folly::toPrettyJson(merged);
  };

  // Merge each staged domain. Conflicts from both are aggregated so the user
  // sees every conflicting path at once.
  std::optional<std::string> agentMerged =
      mergeStaged("cli/agent.conf", getSessionConfigPath());
  std::optional<std::string> bgpMerged =
      mergeStaged(kBgpGitRelPath, getBgpSessionConfigPath());

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

  // Write the merged config(s) back to the session file(s).
  if (agentMerged) {
    folly::writeFileAtomic(
        getSessionConfigPath(), *agentMerged, 0644, folly::SyncType::WITH_SYNC);
  }
  if (bgpMerged) {
    folly::writeFileAtomic(
        getBgpSessionConfigPath(),
        *bgpMerged,
        0644,
        folly::SyncType::WITH_SYNC);
  }

  // Update the base to current HEAD (single repo, shared across domains).
  base_ = currentHead;
  saveMetadata();

  // Reload in-memory state for whichever domains were rebased (null == reload
  // on next access).
  if (agentMerged) {
    loadConfig();
  }
  if (bgpMerged) {
    bgpConfig_.reset();
  }
}

std::map<cli::ServiceType, cli::ConfigActionLevel>
ConfigSession::rolledBackActionLevels(const std::string& resolvedSha) const {
  std::map<cli::ServiceType, cli::ConfigActionLevel> levels;
  for (const auto& commit : git_->log(getSystemMetadataPath())) {
    if (commit.sha1 == resolvedSha) {
      return levels;
    }
    try {
      folly::dynamic json = folly::parseJson(
          git_->fileAtRevision(commit.sha1, kMetadataGitRelPath));
      cli::ConfigSessionMetadata metadata;
      facebook::thrift::from_dynamic(
          metadata,
          json,
          facebook::thrift::dynamic_format::PORTABLE,
          facebook::thrift::format_adherence::LENIENT);
      for (const auto& [service, level] : *metadata.action()) {
        auto it = levels.find(service);
        if (it == levels.end() ||
            static_cast<int>(level) > static_cast<int>(it->second)) {
          levels[service] = level;
        }
      }
    } catch (const std::exception& ex) {
      throw std::runtime_error(
          fmt::format(
              "Cannot safely rollback: failed to read metadata at revision "
              "{}: {}",
              Git::shortSha1(commit.sha1),
              ex.what()));
    }
  }
  // resolvedSha never touched the metadata file (or predates it): every
  // metadata-bearing commit was scanned, which is the conservative answer.
  return levels;
}

std::string ConfigSession::rollback(const HostInfo& hostInfo) {
  // Find the previous commit using the metadata file's history. The metadata
  // (cli/cli_metadata.json) is committed by every config commit -- agent OR
  // BGP -- whereas cli/agent.conf is unchanged by a BGP-only commit. Logging
  // the metadata path therefore includes BGP-only commits, so no-arg rollback
  // doesn't silently skip them.
  auto commits = git_->log(getSystemMetadataPath(), 2);
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
  ensureDirectoryExists(getCliConfigDir());

  // Resolve the commit SHA (in case it's a short SHA or ref)
  std::string resolvedSha = git_->resolveRef(commitSha);

  // Read the target metadata; this is present in every commit, so it also
  // validates the revision (a bad ref throws here and propagates).
  std::string metadataPath = getSystemMetadataPath();
  std::string targetMetadataData =
      git_->fileAtRevision(resolvedSha, kMetadataGitRelPath);
  std::string oldMetadataData;
  if (fs::exists(metadataPath)) {
    if (!folly::readFile(metadataPath.c_str(), oldMetadataData)) {
      throw std::runtime_error(
          fmt::format("Failed to read current metadata from {}", metadataPath));
    }
  }

  // Per-domain: target content at the revision vs the currently-promoted
  // content. A rollback only acts on a domain whose config actually changes
  // (a BGP-only commit leaves cli/agent.conf identical, and vice versa).
  struct DomainRollback {
    ConfigDomain domain;
    std::string target;
    std::string oldPromoted;
    bool promotedExisted;
    bool changed;
  };
  std::vector<DomainRollback> doms;
  for (const auto& domain : configDomains()) {
    // fileAtRevisionOrEmpty: a path absent at the revision (e.g. bgpcpp.conf
    // before BGP existed) is treated as empty content -> remove on rollback.
    std::string target = fileAtRevisionOrEmpty(resolvedSha, domain.gitRelPath);
    std::string oldPromoted = readPromotedContent(domain);
    bool existed = fs::exists(domain.promotedPath);
    bool changed = !domainContentEqual(domain, target, oldPromoted);
    doms.push_back(
        {domain, std::move(target), std::move(oldPromoted), existed, changed});
  }

  // Reload/restart only the services whose config changed. Each starts at its
  // domain's default rollback action level and is promoted to the highest
  // level recorded by any commit being undone: undoing a change needs at least
  // the action applying it did (e.g. a VLAN membership change cannot be
  // applied with a hitless reload in either direction). Computed before any
  // file is touched so a git failure here aborts cleanly.
  auto recordedLevels = rolledBackActionLevels(resolvedSha);
  std::map<cli::ServiceType, cli::ConfigActionLevel> actions;
  for (const auto& dr : doms) {
    if (!dr.changed) {
      continue;
    }
    auto level = dr.domain.rollbackActionLevel;
    auto it = recordedLevels.find(dr.domain.service);
    if (it != recordedLevels.end() &&
        static_cast<int>(it->second) > static_cast<int>(level)) {
      level = it->second;
    }
    actions[dr.domain.service] = level;
  }

  // The rollback commit's metadata must record the actions IT applied, not the
  // target commit's: a later rollback undoing this one crosses the same
  // changes and reads this action map to pick its own level.
  try {
    folly::dynamic json = folly::parseJson(targetMetadataData);
    cli::ConfigSessionMetadata metadata;
    facebook::thrift::from_dynamic(
        metadata,
        json,
        facebook::thrift::dynamic_format::PORTABLE,
        facebook::thrift::format_adherence::LENIENT);
    metadata.action() = actions;
    targetMetadataData = folly::toPrettyJson(
        facebook::thrift::to_dynamic(
            metadata, facebook::thrift::dynamic_format::PORTABLE));
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        fmt::format(
            "Cannot safely rollback to {}: failed to parse target metadata: "
            "{}",
            Git::shortSha1(resolvedSha),
            ex.what()));
  }

  // Always restore the metadata (it records the new rollback base). Promote
  // each changed domain to its target (or remove its file if the domain didn't
  // exist at that revision), leaving unchanged domains untouched to avoid
  // needless writes and symlink churn.
  folly::writeFileAtomic(
      metadataPath, targetMetadataData, 0644, folly::SyncType::WITH_SYNC);
  std::vector<std::string> rollbackFiles = {metadataPath};
  for (const auto& dr : doms) {
    if (!dr.changed) {
      continue;
    }
    if (!dr.target.empty()) {
      promoteDomain(dr.domain, dr.target, rollbackFiles);
    } else if (dr.promotedExisted) {
      std::error_code rmEc;
      fs::remove(dr.domain.promotedPath, rmEc);
      if (rmEc) {
        throw std::runtime_error(
            fmt::format(
                "Failed to remove {} while rolling back to a revision that "
                "predates it: {}",
                dr.domain.promotedPath,
                rmEc.message()));
      }
    }
  }

  // Apply the rolled-back config - if this fails, restore prior state.
  std::string newCommitSha;
  try {
    applyServiceActions(actions, hostInfo);

    std::string commitMessage = fmt::format(
        "Rollback to {} by {}", Git::shortSha1(resolvedSha), username_);
    newCommitSha = git_->commit(rollbackFiles, commitMessage, username_, "");
    LOG(INFO) << "Rollback committed as " << Git::shortSha1(newCommitSha);
  } catch (const std::exception& ex) {
    // Restore the old metadata and each changed domain's config.
    try {
      if (!oldMetadataData.empty()) {
        folly::writeFileAtomic(
            metadataPath, oldMetadataData, 0644, folly::SyncType::WITH_SYNC);
      }
      for (const auto& dr : doms) {
        if (dr.changed) {
          restorePromotedDomain(dr.domain, dr.oldPromoted, dr.promotedExisted);
        }
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

  // The on-disk config changed underneath any cached in-memory state; force a
  // reload on next access (null == not loaded) regardless of session
  // cleanliness.
  agentConfig_.reset();
  bgpConfig_.reset();

  // Update the session state after rollback
  // Check if the current session is clean (no pending changes)
  if (commands_.empty()) {
    // Session is clean - update base to the new rollback commit and sync any
    // active session config to match the rolled-back configuration.
    base_ = newCommitSha;

    // Only re-seed a session file that already exists, and seed each domain
    // from its own rolled-back data. Unconditionally writing the agent session
    // file would materialize a phantom agent session after a BGP-only rollback
    // (and leave the BGP session file stale); keep the two domains symmetric.
    for (const auto& dr : doms) {
      if (!fs::exists(dr.domain.sessionPath)) {
        continue;
      }
      if (!dr.target.empty()) {
        folly::writeFileAtomic(
            dr.domain.sessionPath, dr.target, 0644, folly::SyncType::WITH_SYNC);
      } else {
        std::error_code rmEc;
        fs::remove(dr.domain.sessionPath, rmEc);
      }
    }

    // Save the updated metadata (with new base)
    saveMetadata();

    LOG(INFO) << "Session updated to rollback commit "
              << Git::shortSha1(newCommitSha);
  } else {
    // Session has pending changes - warn that it needs to be rebased
    LOG(WARNING) << fmt::format(
        "Current session contains {} pending change(s) and needs to be rebased. "
        "Run 'fboss2-dev config session rebase' or 'fboss2-dev config session clear' "
        "before making new changes.",
        commands_.size());
  }

  return newCommitSha;
}

} // namespace facebook::fboss
