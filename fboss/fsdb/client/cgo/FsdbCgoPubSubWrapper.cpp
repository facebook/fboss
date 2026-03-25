// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/cgo/FsdbCgoPubSubWrapper.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <stdexcept>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

namespace facebook::fboss::fsdb {

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

// Subscribe to Agent stats
void FsdbCgoPubSubWrapper::subscribeStatsPath(std::optional<int> serverPort) {
  if (statsSubscribed_) {
    XLOG(WARNING) << "Already subscribed to stats path";
    return;
  }

  const std::vector<std::string> path = {"agent"};

  auto subscriptionStateCb = [](fsdb::SubscriptionState oldState,
                                fsdb::SubscriptionState newState,
                                std::optional<bool> /*initialSyncHasData*/) {
    XLOG(INFO) << "FSDB stats subscription changed from "
               << subscriptionStateToString(oldState) << " to "
               << subscriptionStateToString(newState);
  };

  auto operStatsCb = [this, path](fsdb::OperState&& operState) {
    if (!operState.contents()) {
      XLOG(WARNING) << "Received empty OperState for stats path";
      return;
    }

    try {
      facebook::fboss::AgentStats agentStats =
          apache::thrift::BinarySerializer::deserialize<
              facebook::fboss::AgentStats>(*operState.contents());

      std::string key = folly::join(".", path);

      enqueueStats(key, std::move(agentStats));
    } catch (const std::exception& e) {
      XLOG(ERR) << "Failed to deserialize AgentStats: " << e.what();
    }
  };

  try {
    if (serverPort.has_value()) {
      utils::ConnectionOptions connOptions("::1", *serverPort);
      pubSubMgr_->addStatPathSubscription(
          path,
          std::move(subscriptionStateCb),
          std::move(operStatsCb),
          std::move(connOptions));

      XLOG(INFO) << "Successfully subscribed to stats path on port "
                 << *serverPort;
    } else {
      pubSubMgr_->addStatPathSubscription(
          path, std::move(subscriptionStateCb), std::move(operStatsCb));

      XLOG(INFO) << "Successfully subscribed to stats path (default port)";
    }

    statsSubscribed_ = true;
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to subscribe to stats path: " << e.what();
    throw std::runtime_error(
        std::string("Failed to subscribe to stats path: ") + e.what());
  }
}

std::unordered_map<std::string, bool>
FsdbCgoPubSubWrapper::waitForStateUpdates() {
  if (!stateSubscribed_.load()) {
    throw std::runtime_error("No active state subscription");
  }

  // Wait for at least one state update (blocks)
  std::tuple<std::string, bool> update;
  stateQueue_.dequeue(update);

  // Collect all available state updates into a map
  std::unordered_map<std::string, bool> updates;
  updates.insert_or_assign(std::get<0>(update), std::get<1>(update));

  // Try to get more updates without blocking
  while (stateQueue_.try_dequeue(update)) {
    updates.insert_or_assign(std::get<0>(update), std::get<1>(update));
  }

  XLOG(DBG2) << "Returning " << updates.size() << " state updates";
  return updates;
}

std::unordered_map<std::string, facebook::fboss::AgentStats>
FsdbCgoPubSubWrapper::waitForStatsUpdates() {
  if (!statsSubscribed_.load()) {
    throw std::runtime_error("No active stats subscription");
  }

  // Wait for at least one stats update (blocks)
  std::tuple<std::string, facebook::fboss::AgentStats> update;
  statsQueue_.dequeue(update);

  // Collect all available stats updates into a map
  std::unordered_map<std::string, facebook::fboss::AgentStats> updates;
  updates.insert_or_assign(std::get<0>(update), std::move(std::get<1>(update)));

  // Try to get more updates without blocking
  while (statsQueue_.try_dequeue(update)) {
    updates.insert_or_assign(
        std::get<0>(update), std::move(std::get<1>(update)));
  }

  XLOG(DBG2) << "Returning " << updates.size() << " stats updates";
  return updates;
}

void FsdbCgoPubSubWrapper::enqueueState(
    const std::string& key,
    bool portOperState) {
  XLOG(DBG2) << "Received state update for key: " << key
             << ", portOperState: " << (portOperState ? "UP" : "DOWN");

  // Enqueue to state queue
  stateQueue_.enqueue(std::make_tuple(key, portOperState));
}

void FsdbCgoPubSubWrapper::enqueueStats(
    const std::string& key,
    facebook::fboss::AgentStats&& stats) {
  XLOG(DBG2) << "Received stats update for key: " << key;

  // Enqueue to stats queue
  statsQueue_.enqueue(std::make_tuple(key, std::move(stats)));
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

// Opaque handle type for Go
typedef void* FsdbWrapperHandle;

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

/**
 * Subscribe to stats path (hardcoded to "agent") using default connection
 * @param handle Opaque handle to the wrapper
 */
void SubscribeToStatsPath(FsdbWrapperHandle handle) {
  if (!handle) {
    return;
  }

  try {
    auto* wrapper = static_cast<FsdbCgoPubSubWrapper*>(handle);
    wrapper->subscribeStatsPath();
  } catch (const std::exception&) {
    // Subscription failed - error already logged
  }
}

/**
 * Check if state subscription is active
 * @param handle Opaque handle to the wrapper
 * @return 1 if active, 0 otherwise
 */
int HasStateSubscription(FsdbWrapperHandle handle) {
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
int HasStatsSubscription(FsdbWrapperHandle handle) {
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

} // extern "C"

} // namespace facebook::fboss::fsdb
