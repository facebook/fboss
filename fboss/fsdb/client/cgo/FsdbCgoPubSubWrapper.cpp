// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/cgo/FsdbCgoPubSubWrapper.h"

#include <folly/String.h>
#include <folly/init/Init.h>
#include <folly/init/Phase.h>
#include <folly/logging/xlog.h>
#include <folly/synchronization/CallOnce.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <chrono>
#include <limits>
#include <stdexcept>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

namespace facebook::fboss::fsdb {

namespace {
// Worst-case shutdown latency for waitFor*.
constexpr auto kPollInterval = std::chrono::milliseconds(100);
// Bounds producer-side backpressure on a full queue. Past this, the FSDB
// callback drops the update with an error log instead of stalling indefinitely.
constexpr auto kEnqueueTimeout = std::chrono::milliseconds(100);

// Polls `shuttingDown` every kPollInterval until first item or shutdown,
// then non-blocking-drain up to `maxCount`. Empty vector on shutdown.
template <typename T, typename Queue>
std::vector<T> waitAndDrain(
    Queue& queue,
    std::atomic<bool>& shuttingDown,
    int maxCount,
    size_t batchHint) {
  if (maxCount <= 0) {
    return {};
  }
  T update;
  while (!shuttingDown.load(std::memory_order_acquire)) {
    if (queue.try_dequeue_for(update, kPollInterval)) {
      break;
    }
  }
  if (shuttingDown.load(std::memory_order_acquire)) {
    return {};
  }
  std::vector<T> updates;
  updates.reserve(std::min(static_cast<size_t>(maxCount), batchHint));
  updates.emplace_back(std::move(update));
  while (static_cast<int>(updates.size()) < maxCount &&
         queue.try_dequeue(update)) {
    updates.emplace_back(std::move(update));
  }
  return updates;
}
} // namespace

FsdbCgoPubSubWrapper::FsdbCgoPubSubWrapper(const std::string& clientId)
    : clientId_(clientId) {
  XLOG(INFO) << "FsdbCgoPubSubWrapper created for client: " << clientId_;

  pubSubMgr_ = std::make_unique<FsdbPubSubManager>(clientId_);
}

FsdbCgoPubSubWrapper::~FsdbCgoPubSubWrapper() {
  XLOG(INFO) << "FsdbCgoPubSubWrapper destructor for client: " << clientId_;

  // FsdbPubSubManager will automatically clean up all subscriptions
}

void FsdbCgoPubSubWrapper::subscribeToOperState_portMaps(
    std::optional<int> serverPort) {
  if (stateSubscribed_) {
    XLOG(WARNING) << "Already subscribed to portMaps state";
    return;
  }

  thriftpath::RootThriftPath<FsdbOperStateRoot> rootPath;
  auto path = rootPath.agent().switchState().portMaps();

  auto subscriptionStateCb = [](fsdb::SubscriptionState oldState,
                                fsdb::SubscriptionState newState,
                                std::optional<bool> /*initialSyncHasData*/) {
    XLOG(INFO) << "FSDB state subscription changed from "
               << subscriptionStateToString(oldState) << " to "
               << subscriptionStateToString(newState);
  };

  auto operStateCb = [this](fsdb::OperState&& operState) {
    processPortMapsUpdate(std::move(operState));
  };

  try {
    if (serverPort.has_value()) {
      utils::ConnectionOptions connOptions("::1", *serverPort);
      pubSubMgr_->addStatePathSubscription(
          path.tokens(),
          std::move(subscriptionStateCb),
          std::move(operStateCb),
          std::move(connOptions));

      XLOG(INFO) << "Successfully subscribed to portMaps state on port "
                 << *serverPort;
    } else {
      pubSubMgr_->addStatePathSubscription(
          path.tokens(),
          std::move(subscriptionStateCb),
          std::move(operStateCb));

      XLOG(INFO) << "Successfully subscribed to portMaps state (default port)";
    }

    stateSubscribed_ = true;
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to subscribe to portMaps state: " << e.what();
    throw std::runtime_error(
        std::string("Failed to subscribe to portMaps state: ") + e.what());
  }
}

void FsdbCgoPubSubWrapper::subscribeStatsPath(
    const std::vector<std::string>& path,
    std::optional<int> serverPort) {
  if (statsSubscribed_) {
    XLOG(WARNING) << "Already subscribed to a stats path; ignoring";
    return;
  }
  if (path.empty()) {
    throw std::runtime_error("subscribeStatsPath: empty path");
  }

  const std::string pathStr = folly::join(".", path);

  auto subscriptionStateCb = [pathStr](
                                 fsdb::SubscriptionState oldState,
                                 fsdb::SubscriptionState newState,
                                 std::optional<bool> /*initialSyncHasData*/) {
    XLOG(INFO) << "FSDB stats subscription (path=" << pathStr
               << ") changed from " << subscriptionStateToString(oldState)
               << " to " << subscriptionStateToString(newState);
  };

  auto operStatsCb = [this, pathStr](fsdb::OperState&& operState) {
    if (!operState.contents()) {
      XLOG(WARNING) << "Empty OperState for stats path " << pathStr;
      return;
    }
    const int32_t protocol = static_cast<int32_t>(*operState.protocol());
    enqueueStats(pathStr, std::move(*operState.contents()), protocol);
  };

  try {
    if (serverPort.has_value()) {
      utils::ConnectionOptions connOptions("::1", *serverPort);
      pubSubMgr_->addStatPathSubscription(
          path,
          std::move(subscriptionStateCb),
          std::move(operStatsCb),
          std::move(connOptions));

      XLOG(INFO) << "Successfully subscribed to stats path " << pathStr
                 << " on port " << *serverPort;
    } else {
      pubSubMgr_->addStatPathSubscription(
          path, std::move(subscriptionStateCb), std::move(operStatsCb));

      XLOG(INFO) << "Successfully subscribed to stats path " << pathStr
                 << " (default port)";
    }

    statsSubscribed_ = true;
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to subscribe to stats path " << pathStr << ": "
              << e.what();
    throw std::runtime_error(
        std::string("Failed to subscribe to stats path: ") + e.what());
  }
}

std::vector<std::tuple<std::string, bool>>
FsdbCgoPubSubWrapper::waitForStateUpdates(int maxCount) {
  if (!stateSubscribed_.load()) {
    throw std::runtime_error("No active state subscription");
  }
  constexpr size_t kStateBatchHint = 128;
  auto updates = waitAndDrain<std::tuple<std::string, bool>>(
      stateQueue_, shuttingDown_, maxCount, kStateBatchHint);
  XLOG(DBG2) << "Returning " << updates.size() << " state updates";
  return updates;
}

std::vector<std::tuple<std::string, folly::fbstring, int32_t>>
FsdbCgoPubSubWrapper::waitForStatsUpdates(int maxCount) {
  if (!statsSubscribed_.load()) {
    throw std::runtime_error("No active stats subscription");
  }
  constexpr size_t kStatsBatchHint = 16;
  auto updates =
      waitAndDrain<std::tuple<std::string, folly::fbstring, int32_t>>(
          statsQueue_, shuttingDown_, maxCount, kStatsBatchHint);
  XLOG(DBG2) << "Returning " << updates.size() << " stats updates";
  return updates;
}

void FsdbCgoPubSubWrapper::enqueueState(
    const std::string& key,
    bool portOperState) {
  XLOG(DBG2) << "Received state update for key: " << key
             << ", portOperState: " << (portOperState ? "UP" : "DOWN");

  if (!stateQueue_.try_enqueue_for(
          std::make_tuple(key, portOperState), kEnqueueTimeout)) {
    XLOG(ERR) << "stateQueue_ full after " << kEnqueueTimeout.count()
              << "ms; dropping update for key: " << key;
  }
}

void FsdbCgoPubSubWrapper::enqueueStats(
    const std::string& key,
    folly::fbstring&& contents,
    int32_t protocol) {
  XLOG(DBG2) << "Received stats update for key: " << key << " ("
             << contents.size() << " bytes, protocol=" << protocol << ")";

  if (!statsQueue_.try_enqueue_for(
          std::make_tuple(key, std::move(contents), protocol),
          kEnqueueTimeout)) {
    XLOG(ERR) << "statsQueue_ full after " << kEnqueueTimeout.count()
              << "ms; dropping update for key: " << key;
  }
}

void FsdbCgoPubSubWrapper::subscribeStatePath(
    const std::vector<std::string>& path,
    std::optional<int> serverPort) {
  if (statePathSubscribed_) {
    XLOG(WARNING) << "Already subscribed to a state path; ignoring";
    return;
  }
  if (path.empty()) {
    throw std::runtime_error("subscribeStatePath: empty path");
  }

  const std::string pathStr = folly::join(".", path);

  auto subscriptionStateCb = [pathStr](
                                 fsdb::SubscriptionState oldState,
                                 fsdb::SubscriptionState newState,
                                 std::optional<bool> /*initialSyncHasData*/) {
    XLOG(INFO) << "FSDB state-path subscription (path=" << pathStr
               << ") changed from " << subscriptionStateToString(oldState)
               << " to " << subscriptionStateToString(newState);
  };

  auto operStateCb = [this, pathStr](fsdb::OperState&& operState) {
    if (!operState.contents()) {
      XLOG(WARNING) << "Empty OperState for state path " << pathStr;
      return;
    }
    const int32_t protocol = static_cast<int32_t>(*operState.protocol());
    enqueueStatePath(pathStr, std::move(*operState.contents()), protocol);
  };

  try {
    if (serverPort.has_value()) {
      utils::ConnectionOptions connOptions("::1", *serverPort);
      pubSubMgr_->addStatePathSubscription(
          path,
          std::move(subscriptionStateCb),
          std::move(operStateCb),
          std::move(connOptions));

      XLOG(INFO) << "Successfully subscribed to state path " << pathStr
                 << " on port " << *serverPort;
    } else {
      pubSubMgr_->addStatePathSubscription(
          path, std::move(subscriptionStateCb), std::move(operStateCb));

      XLOG(INFO) << "Successfully subscribed to state path " << pathStr
                 << " (default port)";
    }

    statePathSubscribed_ = true;
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to subscribe to state path " << pathStr << ": "
              << e.what();
    throw std::runtime_error(
        std::string("Failed to subscribe to state path: ") + e.what());
  }
}

std::vector<std::tuple<std::string, folly::fbstring, int32_t>>
FsdbCgoPubSubWrapper::waitForStatePathUpdates(int maxCount) {
  if (!statePathSubscribed_.load()) {
    throw std::runtime_error("No active state-path subscription");
  }
  constexpr size_t kStatePathBatchHint = 128;
  auto updates =
      waitAndDrain<std::tuple<std::string, folly::fbstring, int32_t>>(
          statePathQueue_, shuttingDown_, maxCount, kStatePathBatchHint);
  XLOG(DBG2) << "Returning " << updates.size() << " state-path updates";
  return updates;
}

void FsdbCgoPubSubWrapper::enqueueStatePath(
    const std::string& key,
    folly::fbstring&& contents,
    int32_t protocol) {
  XLOG(DBG2) << "Received state-path update for key: " << key << " ("
             << contents.size() << " bytes, protocol=" << protocol << ")";

  if (!statePathQueue_.try_enqueue_for(
          std::make_tuple(key, std::move(contents), protocol),
          kEnqueueTimeout)) {
    XLOG(ERR) << "statePathQueue_ full after " << kEnqueueTimeout.count()
              << "ms; dropping update for key: " << key;
  }
}

void FsdbCgoPubSubWrapper::processPortMapsUpdate(fsdb::OperState&& operState) {
  if (!operState.contents()) {
    XLOG(WARNING) << "Received empty OperState for portMaps";
    return;
  }

  try {
    using PortMaps = std::
        map<std::string, std::map<int16_t, facebook::fboss::state::PortFields>>;

    PortMaps portMaps = apache::thrift::BinarySerializer::deserialize<PortMaps>(
        *operState.contents());

    // Process each port and check if state changed
    for (const auto& [switchId, portMap] : portMaps) {
      for (const auto& [portId, portFields] : portMap) {
        // Use port name as the key
        const auto& portName = *portFields.portName();
        bool currentState = *portFields.portOperState();

        // Enqueue update if this is a new port or if state changed
        auto it = portName2OperState_.find(portName);
        if (it == portName2OperState_.end() || it->second != currentState) {
          enqueueState(portName, currentState);
          portName2OperState_[portName] = currentState;
        }
      }
    }
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to deserialize and process portMaps: " << e.what();
  }
}

// ============================================================================
// C API for CGO (Go<->C++ interop)
// ============================================================================
// These functions provide a C-compatible interface that Go can call via CGO

extern "C" {

/**
 * Create a new FsdbCgoPubSubWrapper instance
 * @param clientId Client identifier string
 * @return Opaque handle to the wrapper, or nullptr on failure
 */
FsdbWrapperHandle FOLLY_NULLABLE CreateFsdbWrapper(const char* clientId) {
  if (!clientId) {
    return nullptr;
  }

  try {
    auto* wrapper = new FsdbCgoPubSubWrapper(std::string(clientId));
    return static_cast<FsdbWrapperHandle>(wrapper);
  } catch (const std::exception&) {
    return nullptr;
  }
}

/**
 * Destroy a FsdbCgoPubSubWrapper instance
 * @param handle Opaque handle to the wrapper
 */
void DestroyFsdbWrapper(FsdbWrapperHandle handle) {
  if (!handle) {
    return;
  }

  auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
  delete wrapper;
}

// Wakes blocked WaitFor* within ~100 ms. Call before Destroy. Returns 0/1.
int32_t ShutdownFsdbWrapper(FsdbWrapperHandle handle) {
  if (!handle) {
    return 0;
  }
  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    wrapper->shutdown();
    return 0;
  } catch (const std::exception& e) {
    XLOG(ERR) << "ShutdownFsdbWrapper error: " << e.what();
    return 1;
  }
}

/**
 * Subscribe to portMaps state using default connection
 * @param handle Opaque handle to the wrapper
 */
void SubscribeToPortMaps(FsdbWrapperHandle handle) {
  if (!handle) {
    return;
  }

  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    wrapper->subscribeToOperState_portMaps();
  } catch (const std::exception&) {
    // Subscription failed - error already logged
  }
}

static std::vector<std::string> tokensToPath(
    const char** path_tokens,
    int32_t num_tokens) {
  std::vector<std::string> path;
  path.reserve(num_tokens);
  for (int32_t i = 0; i < num_tokens; ++i) {
    if (path_tokens[i] == nullptr) {
      throw std::runtime_error(
          "tokensToPath: null token at index " + std::to_string(i));
    }
    path.emplace_back(path_tokens[i]);
  }
  return path;
}

void SubscribeToStatsPath(
    FsdbWrapperHandle handle,
    const char** path_tokens,
    int32_t num_tokens) {
  if (!handle || !path_tokens || num_tokens <= 0) {
    return;
  }
  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    wrapper->subscribeStatsPath(tokensToPath(path_tokens, num_tokens));
  } catch (const std::exception&) {
    // Subscription failed - error already logged
  }
}

void SubscribeToStatePath(
    FsdbWrapperHandle handle,
    const char** path_tokens,
    int32_t num_tokens) {
  if (!handle || !path_tokens || num_tokens <= 0) {
    return;
  }
  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    wrapper->subscribeStatePath(tokensToPath(path_tokens, num_tokens));
  } catch (const std::exception&) {
    // Subscription failed - error already logged
  }
}

/**
 * Check if state subscription is active
 * @param handle Opaque handle to the wrapper
 * @return 1 if active, 0 otherwise
 */
int32_t HasStateSubscription(FsdbWrapperHandle handle) {
  if (!handle) {
    return 0;
  }

  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    return wrapper->hasStateSubscription() ? 1 : 0;
  } catch (const std::exception&) {
    return 0;
  }
}

/**
 * Check if stats subscription is active
 * @param handle Opaque handle to the wrapper
 * @return 1 if active, 0 otherwise
 */
int32_t HasStatsSubscription(FsdbWrapperHandle handle) {
  if (!handle) {
    return 0;
  }

  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    return wrapper->hasStatsSubscription() ? 1 : 0;
  } catch (const std::exception&) {
    return 0;
  }
}

int32_t HasStatePathSubscription(FsdbWrapperHandle handle) {
  if (!handle) {
    return 0;
  }
  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    return wrapper->hasStatePathSubscription() ? 1 : 0;
  } catch (const std::exception&) {
    return 0;
  }
}

/**
 * Get client ID
 * @param handle Opaque handle to the wrapper
 * @return Client ID string (do not free - owned by wrapper), or nullptr on
 * failure
 */
const char* FOLLY_NULLABLE GetClientId(FsdbWrapperHandle handle) {
  if (!handle) {
    return nullptr;
  }

  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    // Return pointer to internal string - valid until wrapper is destroyed
    return wrapper->getClientId().c_str();
  } catch (const std::exception&) {
    return nullptr;
  }
}

int32_t FsdbCgoAbiVersion() {
  return FSDB_CGO_ABI_VERSION;
}

// ABI-checked folly::Init bootstrap. Idempotent. 0=ok, 1=caught exception,
// 2=ABI mismatch.
int32_t FsdbInit(int32_t consumer_abi_version) {
  if (consumer_abi_version != FSDB_CGO_ABI_VERSION) {
    XLOG(ERR) << "FsdbInit: ABI mismatch — consumer compiled against version "
              << consumer_abi_version << ", library is version "
              << FSDB_CGO_ABI_VERSION
              << ". Aborting init; do not call any other entry point.";
    return 2;
  }
  try {
    static folly::once_flag initFlag;
    folly::call_once(initFlag, []() {
      // Check if folly is already initialized (e.g. by test framework)
      if (folly::get_process_phase() != folly::ProcessPhase::Init) {
        XLOG(INFO) << "FsdbInit: Folly already initialized, skipping";
        return;
      }
      static std::unique_ptr<folly::Init> follyInit;
      int argc = 1;
      const char* argv0 = "fsdb_cgo";
      char* mutableArgv0 = const_cast<char*>(argv0);
      char** argv = &mutableArgv0;
      follyInit = std::make_unique<folly::Init>(&argc, &argv, false);
      XLOG(INFO) << "FsdbInit: Folly runtime initialized";
    });
    return 0;
  } catch (const std::exception& e) {
    XLOG(ERR) << "FsdbInit failed: " << e.what();
    return 1;
  }
}

// Returns count, 0 on shutdown, -1 on error. Surplus stays queued.
int32_t WaitForStateUpdates(
    FsdbWrapperHandle handle,
    FsdbPortStateUpdate* out,
    int32_t max_count) {
  if (!handle || !out || max_count <= 0) {
    return -1;
  }

  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    if (!wrapper->hasStateSubscription()) {
      return -1;
    }

    // Stash on wrapper so c_str() pointers stay live across the C boundary.
    wrapper->lastStateUpdates_ = wrapper->waitForStateUpdates(max_count);

    int32_t i = 0;
    for (const auto& [portName, operState] : wrapper->lastStateUpdates_) {
      out[i].port_name = portName.c_str();
      out[i].oper_state = operState ? 1 : 0;
      ++i;
    }
    return i;
  } catch (const std::exception& e) {
    XLOG(ERR) << "WaitForStateUpdates error: " << e.what();
    return -1;
  }
}

int32_t WaitForStatsUpdates(
    FsdbWrapperHandle handle,
    FsdbStatsUpdate* out,
    int32_t max_count) {
  if (!handle || !out || max_count <= 0) {
    return -1;
  }

  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    if (!wrapper->hasStatsSubscription()) {
      return -1;
    }

    wrapper->lastStatsUpdates_ = wrapper->waitForStatsUpdates(max_count);

    int32_t i = 0;
    for (const auto& [key, contents, protocol] : wrapper->lastStatsUpdates_) {
      if (contents.size() >
          static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
        XLOG(ERR) << "Stats update for key '" << key << "' is "
                  << contents.size() << " bytes (>INT32_MAX); dropping";
        continue;
      }
      out[i].key = key.c_str();
      out[i].data = reinterpret_cast<const uint8_t*>(contents.data());
      out[i].data_len = static_cast<int32_t>(contents.size());
      out[i].protocol = protocol;
      ++i;
    }
    return i;
  } catch (const std::exception& e) {
    XLOG(ERR) << "WaitForStatsUpdates error: " << e.what();
    return -1;
  }
}

/**
 * Free the state updates buffer (invalidates borrowed pointers from
 * WaitForStateUpdates)
 * @param handle Opaque handle to the wrapper
 */
void FreeStateUpdates(FsdbWrapperHandle handle) {
  if (!handle) {
    return;
  }
  auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
  wrapper->lastStateUpdates_.clear();
}

/**
 * Free the stats updates buffer (invalidates borrowed pointers from
 * WaitForStatsUpdates)
 * @param handle Opaque handle to the wrapper
 */
void FreeStatsUpdates(FsdbWrapperHandle handle) {
  if (!handle) {
    return;
  }
  auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
  wrapper->lastStatsUpdates_.clear();
}

int32_t WaitForStatePathUpdates(
    FsdbWrapperHandle handle,
    FsdbStatePathUpdate* out,
    int32_t max_count) {
  if (!handle || !out || max_count <= 0) {
    return -1;
  }
  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    if (!wrapper->hasStatePathSubscription()) {
      return -1;
    }

    wrapper->lastStatePathUpdates_ =
        wrapper->waitForStatePathUpdates(max_count);

    int32_t i = 0;
    for (const auto& [key, contents, protocol] :
         wrapper->lastStatePathUpdates_) {
      if (contents.size() >
          static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
        XLOG(ERR) << "State-path update for key '" << key << "' is "
                  << contents.size() << " bytes (>INT32_MAX); dropping";
        continue;
      }
      out[i].key = key.c_str();
      out[i].data = reinterpret_cast<const uint8_t*>(contents.data());
      out[i].data_len = static_cast<int32_t>(contents.size());
      out[i].protocol = protocol;
      ++i;
    }
    return i;
  } catch (const std::exception& e) {
    XLOG(ERR) << "WaitForStatePathUpdates error: " << e.what();
    return -1;
  }
}

void FreeStatePathUpdates(FsdbWrapperHandle handle) {
  if (!handle) {
    return;
  }
  auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
  wrapper->lastStatePathUpdates_.clear();
}

/**
 * Subscribe to portMaps state with an explicit server port
 * @param handle Opaque handle to the wrapper
 * @param server_port Port number, or -1 for default
 */
void SubscribeToPortMapsWithPort(
    FsdbWrapperHandle handle,
    int32_t server_port) {
  if (!handle) {
    return;
  }

  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    if (server_port >= 0) {
      wrapper->subscribeToOperState_portMaps(server_port);
    } else {
      wrapper->subscribeToOperState_portMaps(std::nullopt);
    }
  } catch (const std::exception&) {
    // Subscription failed - error already logged
  }
}

void SubscribeToStatsPathWithPort(
    FsdbWrapperHandle handle,
    const char** path_tokens,
    int32_t num_tokens,
    int32_t server_port) {
  if (!handle || !path_tokens || num_tokens <= 0) {
    return;
  }
  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    auto path = tokensToPath(path_tokens, num_tokens);
    if (server_port >= 0) {
      wrapper->subscribeStatsPath(path, server_port);
    } else {
      wrapper->subscribeStatsPath(path, std::nullopt);
    }
  } catch (const std::exception&) {
    // Subscription failed - error already logged
  }
}

void SubscribeToStatePathWithPort(
    FsdbWrapperHandle handle,
    const char** path_tokens,
    int32_t num_tokens,
    int32_t server_port) {
  if (!handle || !path_tokens || num_tokens <= 0) {
    return;
  }
  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    auto path = tokensToPath(path_tokens, num_tokens);
    if (server_port >= 0) {
      wrapper->subscribeStatePath(path, server_port);
    } else {
      wrapper->subscribeStatePath(path, std::nullopt);
    }
  } catch (const std::exception&) {
    // Subscription failed - error already logged
  }
}

} // extern "C"

} // namespace facebook::fboss::fsdb
