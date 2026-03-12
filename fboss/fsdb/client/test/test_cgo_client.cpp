// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/test/test_cgo_client.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <chrono>

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/instantiations/FsdbCowStateSubManager.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

namespace facebook::fboss::fsdb::test {

namespace {

void deleteSubManager(void* ptr) {
  delete static_cast<FsdbCowStateSubManager*>(ptr);
}

std::string serializeOperState(const OperState& state) {
  return apache::thrift::SimpleJSONSerializer::serialize<std::string>(state);
}

std::string serializeOperDelta(const OperDelta& delta) {
  return apache::thrift::SimpleJSONSerializer::serialize<std::string>(delta);
}

// Wrapper to allow using addPath() with runtime string paths
struct RawStatePath {
  using RootT = FsdbOperStateRoot;

  explicit RawStatePath(std::vector<std::string> tokens)
      : tokens_(std::move(tokens)) {}

  std::vector<std::string> idTokens() const {
    return tokens_;
  }

 private:
  std::vector<std::string> tokens_;
};

std::string serializeSubUpdate(
    const FsdbCowStateSubManager::SubUpdate& update) {
  std::string result = "{";
  result += "\"updatedPaths\": [";
  bool first = true;
  for (const auto& path : update.updatedPaths) {
    if (!first) {
      result += ", ";
    }
    result += "\"/" + folly::join("/", path) + "\"";
    first = false;
  }
  result += "]";
  if (update.lastServedAt.has_value()) {
    result += ", \"lastServedAt\": " + std::to_string(*update.lastServedAt);
  }
  if (update.lastPublishedAt.has_value()) {
    result +=
        ", \"lastPublishedAt\": " + std::to_string(*update.lastPublishedAt);
  }
  result += "}";
  return result;
}

// Helper to create OperState from AgentData
OperState makeAgentDataOperState(const AgentData& agentData) {
  OperState stateUnit;
  stateUnit.contents() =
      apache::thrift::BinarySerializer::serialize<std::string>(agentData);
  stateUnit.protocol() = OperProtocol::BINARY;
  return stateUnit;
}

} // namespace

FsdbTestCgoClient::FsdbTestCgoClient(std::string clientId) noexcept
    : clientId_(std::move(clientId)), subManager_(nullptr, deleteSubManager) {
  pubSubManager_ = std::make_unique<FsdbPubSubManager>(clientId_);
}

FsdbTestCgoClient::~FsdbTestCgoClient() {
  unsubscribeAll();
  disconnectPublisher();
  stopInProcessServer();
}

void FsdbTestCgoClient::setServer(std::string hostIp, int32_t fsdbPort) {
  hostIp_ = std::move(hostIp);
  fsdbPort_ = fsdbPort;
}

void FsdbTestCgoClient::startInProcessServer() {
  if (testServer_) {
    return;
  }
  testServer_ = std::make_unique<FsdbTestServer>(0);
  fsdbPort_ = testServer_->getFsdbPort();
  XLOG(INFO) << "Started in-process FSDB test server on port " << fsdbPort_;
}

void FsdbTestCgoClient::stopInProcessServer() {
  if (testServer_) {
    testServer_.reset();
    XLOG(INFO) << "Stopped in-process FSDB test server";
  }
}

int32_t FsdbTestCgoClient::getServerPort() const {
  return fsdbPort_;
}

void FsdbTestCgoClient::subscribeStatePath(std::vector<std::string> path) {
  if (!pathSubscriptionHandle_.empty()) {
    XLOG(WARNING) << "Path subscription already active, unsubscribing first";
    unsubscribeStatePath();
  }

  pathSubscribePath_ = path;

  auto stateChangeCb = [this](
                           SubscriptionState oldState,
                           SubscriptionState newState,
                           std::optional<bool> initialSyncHasData) {
    XLOG(DBG2) << "Path subscription state changed: "
               << static_cast<int>(oldState) << " -> "
               << static_cast<int>(newState);
    pathTracker_.state.store(static_cast<int32_t>(newState));
    if (initialSyncHasData.has_value()) {
      pathTracker_.initialSyncComplete.store(true);
    }
  };

  auto dataCallback = [this](OperState&& state) {
    XLOG(DBG3) << "Received path update";
    pathTracker_.updateCount.fetch_add(1);
    pathTracker_.lastContent.wlock()->assign(serializeOperState(state));
    // Reset and post the baton to allow multiple updates
    updateBaton_.reset();
    updateBaton_.post();
  };

  pathSubscriptionHandle_ = pubSubManager_->addStatePathSubscription(
      path,
      std::move(stateChangeCb),
      std::move(dataCallback),
      utils::ConnectionOptions(hostIp_, fsdbPort_));
  XLOG(INFO) << "Subscribed to state path: " << folly::join("/", path);
}

void FsdbTestCgoClient::unsubscribeStatePath() {
  if (!pathSubscriptionHandle_.empty()) {
    pubSubManager_->removeStatePathSubscription(pathSubscribePath_, hostIp_);
    pathSubscriptionHandle_.clear();
    pathSubscribePath_.clear();
    pathTracker_.state.store(0);
    pathTracker_.initialSyncComplete.store(false);
    XLOG(INFO) << "Unsubscribed from state path";
  }
}

void FsdbTestCgoClient::subscribeStateDelta(std::vector<std::string> path) {
  if (!deltaSubscriptionHandle_.empty()) {
    XLOG(WARNING) << "Delta subscription already active, unsubscribing first";
    unsubscribeStateDelta();
  }

  deltaSubscribePath_ = path;

  auto stateChangeCb = [this](
                           SubscriptionState oldState,
                           SubscriptionState newState,
                           std::optional<bool> initialSyncHasData) {
    XLOG(DBG2) << "Delta subscription state changed: "
               << static_cast<int>(oldState) << " -> "
               << static_cast<int>(newState);
    deltaTracker_.state.store(static_cast<int32_t>(newState));
    if (initialSyncHasData.has_value()) {
      deltaTracker_.initialSyncComplete.store(true);
    }
  };

  auto dataCallback = [this](OperDelta&& delta) {
    XLOG(DBG3) << "Received delta update";
    deltaTracker_.updateCount.fetch_add(1);
    deltaTracker_.lastContent.wlock()->assign(serializeOperDelta(delta));
    // Reset and post the baton to allow multiple updates
    updateBaton_.reset();
    updateBaton_.post();
  };

  deltaSubscriptionHandle_ = pubSubManager_->addStateDeltaSubscription(
      path,
      std::move(stateChangeCb),
      std::move(dataCallback),
      utils::ConnectionOptions(hostIp_, fsdbPort_));
  XLOG(INFO) << "Subscribed to state delta: " << folly::join("/", path);
}

void FsdbTestCgoClient::unsubscribeStateDelta() {
  if (!deltaSubscriptionHandle_.empty()) {
    pubSubManager_->removeStateDeltaSubscription(deltaSubscribePath_, hostIp_);
    deltaSubscriptionHandle_.clear();
    deltaSubscribePath_.clear();
    deltaTracker_.state.store(0);
    deltaTracker_.initialSyncComplete.store(false);
    XLOG(INFO) << "Unsubscribed from state delta";
  }
}

void FsdbTestCgoClient::subscribeStatePatch(std::vector<std::string> path) {
  if (patchSubscriptionActive_) {
    XLOG(WARNING) << "Patch subscription already active, unsubscribing first";
    unsubscribeStatePatch();
  }

  SubscriptionOptions subscriptionOptions(clientId_, false /* isStats */);
  utils::ConnectionOptions connectionOptions(hostIp_, fsdbPort_);

  auto* manager = new FsdbCowStateSubManager(
      std::move(subscriptionOptions), std::move(connectionOptions));
  subManager_.reset(manager);

  RawStatePath rawPath(path);
  manager->addPath(std::move(rawPath));

  auto stateChangeCb = [this](
                           SubscriptionState oldState,
                           SubscriptionState newState,
                           std::optional<bool> initialSyncHasData) {
    XLOG(DBG2) << "Patch subscription state changed: "
               << static_cast<int>(oldState) << " -> "
               << static_cast<int>(newState);
    patchTracker_.state.store(static_cast<int32_t>(newState));
    if (initialSyncHasData.has_value()) {
      patchTracker_.initialSyncComplete.store(true);
    }
  };

  auto dataCallback = [this](FsdbCowStateSubManager::SubUpdate update) {
    XLOG(DBG3) << "Received patch update";
    patchTracker_.updateCount.fetch_add(1);
    patchTracker_.lastContent.wlock()->assign(serializeSubUpdate(update));
    // Reset and post the baton to allow multiple updates
    updateBaton_.reset();
    updateBaton_.post();
  };

  manager->subscribe(std::move(dataCallback), std::move(stateChangeCb));
  patchSubscriptionActive_ = true;
  XLOG(INFO) << "Subscribed to state patch: " << folly::join("/", path);
}

void FsdbTestCgoClient::unsubscribeStatePatch() {
  if (patchSubscriptionActive_) {
    subManager_.reset();
    patchSubscriptionActive_ = false;
    patchTracker_.state.store(0);
    patchTracker_.initialSyncComplete.store(false);
    XLOG(INFO) << "Unsubscribed from state patch";
  }
}

void FsdbTestCgoClient::unsubscribeAll() {
  unsubscribeStatePath();
  unsubscribeStateDelta();
  unsubscribeStatePatch();
}

int32_t FsdbTestCgoClient::getSubscriptionState(
    int32_t subscriptionType) const {
  switch (subscriptionType) {
    case 0:
      return pathTracker_.state.load();
    case 1:
      return deltaTracker_.state.load();
    case 2:
      return patchTracker_.state.load();
    default:
      return 0;
  }
}

bool FsdbTestCgoClient::isInitialSyncComplete(int32_t subscriptionType) const {
  switch (subscriptionType) {
    case 0:
      return pathTracker_.initialSyncComplete.load();
    case 1:
      return deltaTracker_.initialSyncComplete.load();
    case 2:
      return patchTracker_.initialSyncComplete.load();
    default:
      return false;
  }
}

bool FsdbTestCgoClient::waitForUpdate(int64_t timeoutMs) {
  if (timeoutMs <= 0) {
    updateBaton_.wait();
    updateBaton_.reset();
    return true;
  }

  bool result = updateBaton_.try_wait_for(std::chrono::milliseconds(timeoutMs));
  if (result) {
    updateBaton_.reset();
  }
  return result;
}

int64_t FsdbTestCgoClient::getPathUpdateCount() const {
  return pathTracker_.updateCount.load();
}

int64_t FsdbTestCgoClient::getDeltaUpdateCount() const {
  return deltaTracker_.updateCount.load();
}

int64_t FsdbTestCgoClient::getPatchUpdateCount() const {
  return patchTracker_.updateCount.load();
}

std::string FsdbTestCgoClient::getLastPathUpdateContent() const {
  return *pathTracker_.lastContent.rlock();
}

std::string FsdbTestCgoClient::getLastDeltaUpdateContent() const {
  return *deltaTracker_.lastContent.rlock();
}

std::string FsdbTestCgoClient::getLastPatchUpdateContent() const {
  return *patchTracker_.lastContent.rlock();
}

void FsdbTestCgoClient::connectPublisher() {
  if (publisherActive_) {
    XLOG(WARNING) << "Publisher already connected";
    return;
  }

  // Reset connection state
  publisherConnected_.store(false);
  publisherConnectedBaton_.reset();

  // Use "agent" as the publisher path (standard for FSDB)
  publisherPath_ = {"agent"};

  // Create a separate FsdbPubSubManager for publishing with "agent" as client
  // ID FSDB expects publishers to use known client IDs like "agent"
  publisherPubSubManager_ = std::make_unique<FsdbPubSubManager>("agent");

  // Create Path publisher via FsdbPubSubManager
  auto stateChangeCb = [this](
                           FsdbStreamClient::State oldState,
                           FsdbStreamClient::State newState) {
    XLOG(INFO) << "Publisher state changed: " << static_cast<int>(oldState)
               << " -> " << static_cast<int>(newState);
    if (newState == FsdbStreamClient::State::CONNECTED) {
      XLOG(INFO) << "Publisher connected";
      publisherConnected_.store(true);
      publisherConnectedBaton_.post();
    } else if (newState == FsdbStreamClient::State::DISCONNECTED) {
      XLOG(INFO) << "Publisher disconnected";
      publisherConnected_.store(false);
    }
  };

  publisherPubSubManager_->createStatePathPublisher(
      publisherPath_, std::move(stateChangeCb), fsdbPort_);

  publisherActive_ = true;
  XLOG(INFO) << "Publisher connecting to " << hostIp_ << ":" << fsdbPort_;
}

void FsdbTestCgoClient::disconnectPublisher() {
  if (publisherActive_) {
    publisherPubSubManager_->removeStatePathPublisher();
    publisherPubSubManager_.reset();
    publisherActive_ = false;
    publisherConnected_.store(false);
    XLOG(INFO) << "Publisher disconnected";
  }
}

bool FsdbTestCgoClient::isPublisherConnected() const {
  return publisherConnected_.load();
}

bool FsdbTestCgoClient::waitForPublisherConnected(int64_t timeoutMs) {
  if (publisherConnected_.load()) {
    return true;
  }

  if (timeoutMs <= 0) {
    publisherConnectedBaton_.wait();
    return publisherConnected_.load();
  }

  bool result = publisherConnectedBaton_.try_wait_for(
      std::chrono::milliseconds(timeoutMs));
  return result && publisherConnected_.load();
}

bool FsdbTestCgoClient::waitForPublisherReady(int64_t timeoutMs) {
  if (!waitForPublisherConnected(timeoutMs)) {
    return false;
  }

  // Check if the server has registered this publisher
  if (testServer_) {
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < endTime) {
      auto metadata =
          testServer_->getPublisherRootMetadata("agent", false /* isStats */);
      if (metadata.has_value()) {
        XLOG(INFO) << "Publisher ready - registered with server";
        return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    XLOG(WARNING) << "Publisher connected but not ready on server";
    return false;
  }

  // For external servers, just return connected state
  return publisherConnected_.load();
}

void FsdbTestCgoClient::publishTestState(
    std::vector<std::string> /* path */,
    std::string /* jsonValue */) {
  throw std::runtime_error(
      "publishTestState not supported with Path publisher - use typed methods");
}

void FsdbTestCgoClient::publishAgentConfigCmdLineArgs(
    std::string key,
    std::string value) {
  if (!publisherActive_ || !publisherPubSubManager_) {
    throw std::runtime_error(
        "Publisher not connected. Call connectPublisher() first.");
  }

  if (!publisherConnected_.load()) {
    throw std::runtime_error(
        "Publisher not ready. Call waitForPublisherConnected() first.");
  }

  // Create AgentData with the config
  AgentData agentData;
  cfg::AgentConfig config;
  config.defaultCommandLineArgs()[key] = value;
  agentData.config() = std::move(config);

  // Create and publish OperState
  auto operState = makeAgentDataOperState(agentData);
  publisherPubSubManager_->publishState(std::move(operState));

  XLOG(INFO) << "Published agent config command line args: " << key << "="
             << value;
}

void FsdbTestCgoClient::publishAgentSwitchStatePortInfo(
    int64_t switchId,
    int64_t portId,
    std::string portName,
    int32_t operState) {
  if (!publisherActive_ || !publisherPubSubManager_) {
    throw std::runtime_error(
        "Publisher not connected. Call connectPublisher() first.");
  }

  if (!publisherConnected_.load()) {
    throw std::runtime_error(
        "Publisher not ready. Call waitForPublisherConnected() first.");
  }

  // Create PortFields
  state::PortFields port;
  port.portId() = static_cast<int16_t>(portId);
  port.portName() = std::move(portName);
  port.portOperState() = (operState != 0);

  // Create SwitchState with portMaps
  state::SwitchState switchState;
  std::map<int16_t, state::PortFields> portMap;
  portMap[static_cast<int16_t>(portId)] = std::move(port);
  switchState.portMaps()[std::to_string(switchId)] = std::move(portMap);

  // Create AgentData with the switchState
  AgentData agentData;
  agentData.switchState() = std::move(switchState);

  // Create and publish OperState
  auto operState2 = makeAgentDataOperState(agentData);
  publisherPubSubManager_->publishState(std::move(operState2));

  XLOG(INFO) << "Published agent switch state port info: switch=" << switchId
             << ", port=" << portId;
}

} // namespace facebook::fboss::fsdb::test
