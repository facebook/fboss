// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_state_types.h"

#include <type_traits>
#include "fboss/agent/AgentFsdbSyncManager.h"


#include "fboss/lib/TupleUtils.h"

DEFINE_bool(fsdb_sync_full_state, false, "sync whole switch state to fsdb");
DEFINE_bool(
    agent_fsdb_sync,
    true,
    "sync agent state to fsdb, do not turn this off as other services depend on agent state synced to fsdb");
namespace {
const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    fsdbStateRootPath;
const auto kAgentPath = fsdbStateRootPath.agent();

template <typename Name>
bool modifyImpl(
    std::shared_ptr<facebook::fboss::SwitchState>& oldState,
    const std::shared_ptr<facebook::fboss::SwitchState>& newState) {
  CHECK(!oldState->isPublished());
  if (newState->template cref<Name>() != oldState->template cref<Name>()) {
    oldState->template ref<Name>() = newState->template cref<Name>();
    return true;
  }
  return false;
}
} // namespace

namespace facebook::fboss {

// This template is expensive to compile. Extern it and compile as part of
// another cpp file AgentFsdbSyncManager-computeOperDelta.cpp to parallelize the
// build
extern template fsdb::OperDelta
fsdb::computeOperDelta<thrift_cow::ThriftStructNode<fsdb::AgentData>>(
    const std::shared_ptr<thrift_cow::ThriftStructNode<fsdb::AgentData>>&,
    const std::shared_ptr<thrift_cow::ThriftStructNode<fsdb::AgentData>>&,
    const std::vector<std::string>&,
    bool);

AgentFsdbSyncManager::AgentFsdbSyncManager(
    const std::shared_ptr<fsdb::FsdbPubSubManager>& pubSubMgr)
    : fsdb::FsdbSyncManager<fsdb::AgentData>(
          pubSubMgr,
          kAgentPath.tokens(),
          false /* isStats */,
          true /* publishDeltas */) {}

AgentFsdbSyncManager::AgentFsdbSyncManager()
    : fsdb::FsdbSyncManager<fsdb::AgentData>(
          "agent",
          kAgentPath.tokens(),
          false /* isStats */,
          true /* publishDeltas */) {}

void AgentFsdbSyncManager::stateUpdated(const StateDelta& delta) {
  if (!FLAGS_agent_fsdb_sync) {
    return;
  }
  if (!FLAGS_fsdb_sync_full_state) {
    return stateUpdatedDelta(delta);
  }
  std::shared_ptr<SwitchState> newState = delta.newState();
  updateState([newState = std::move(newState)](const auto& agentState) {
    auto newAgentState = agentState->clone();
    newAgentState->template ref<fsdb_model_tags::switchState>() = newState;
    return newAgentState;
  });
}

void AgentFsdbSyncManager::cfgUpdated(
    const cfg::SwitchConfig& oldConfig,
    const cfg::SwitchConfig& newConfig) {
  if (!FLAGS_agent_fsdb_sync) {
    return;
  }
  if (oldConfig == newConfig) {
    return;
  }
  updateState([newConfig](const auto& agentState) {
    auto newAgentState = agentState;
    auto& agentConfig =
        newAgentState->template modify<fsdb_model_tags::config>(&newAgentState);
    auto& switchConfig =
        agentConfig->template modify<agent_config_tags::sw>(&agentConfig);
    // TODO: find a better way to handle this, as this  leads to delta across
    // the whole object
    switchConfig->fromThrift(newConfig);
    return newAgentState;
  });
}

#ifndef IS_OSS
void AgentFsdbSyncManager::bitsflowLockdownLevelUpdated(
    std::optional<cfgr_bitsflow::BitsflowLockdownLevel> oldLevel,
    std::optional<cfgr_bitsflow::BitsflowLockdownLevel> newLevel) {
  if (!FLAGS_agent_fsdb_sync) {
    return;
  }
  if (oldLevel == newLevel) {
    return;
  }
  updateState([newLevel](const auto& agentState) {
    auto newAgentState = agentState;
    auto& bitsflowLockdownLevel =
        newAgentState->template modify<fsdb_model_tags::bitsflowLockdownLevel>(
            &newAgentState);
    if (!newLevel) {
      bitsflowLockdownLevel.reset();
      return newAgentState;
    }
    // this is fine as long as it is just an enum value.
    bitsflowLockdownLevel->fromThrift(newLevel.value());
    return newAgentState;
  });
}
#endif

bool AgentFsdbSyncManager::modify(
    std::shared_ptr<facebook::fboss::SwitchState>& outState,
    const std::shared_ptr<facebook::fboss::SwitchState>& inState) {
  bool changed = false;
  SubscribedMaps maps;
  tupleForEach(
      [&](auto& element) {
        using tag = std::decay_t<decltype(element)>;
        changed |= modifyImpl<tag>(outState, inState);
      },
      maps);
  return changed;
}

void AgentFsdbSyncManager::stateUpdatedDelta(const StateDelta& delta) {
  std::shared_ptr<SwitchState> newState = delta.newState();
  updateState([newState = std::move(newState)](const auto& agentState) {
    bool changed = false;
    // TODO: sync all objects, currently only syncing what fsdb state delta
    // converter supports
    auto newAgentState = agentState->clone();
    auto& switchState =
        newAgentState->template modify<fsdb_model_tags::switchState>();
    changed = AgentFsdbSyncManager::modify(switchState, newState);
    if (!changed) {
      return agentState;
    }
    newAgentState->template ref<fsdb_model_tags::switchState>() = switchState;
    return newAgentState;
  });
}

void AgentFsdbSyncManager::updateDsfSubscriberState(
    const std::string& nodeName,
    fsdb::FsdbSubscriptionState oldState,
    fsdb::FsdbSubscriptionState newState) {
  using subsKey = fsdb_model_tags::fsdbSubscriptions;
  updateState([newState, nodeName](const auto& agentState) {
    const auto& oldDsfSubscriptions = agentState->template safe_cref<subsKey>();
    auto it = oldDsfSubscriptions->find(nodeName);
    if (it != oldDsfSubscriptions->end() && it->second.value() == newState) {
      // no change
      return agentState;
    }

    auto newAgentState = agentState->clone();
    auto& newDsfSubscriptions = newAgentState->template modify<subsKey>();
    if (it == oldDsfSubscriptions->end()) {
      newDsfSubscriptions->emplace(std::move(nodeName), newState);
    } else {
      newDsfSubscriptions->at(std::move(nodeName)) = newState;
    }

    newAgentState->template ref<subsKey>() = std::move(newDsfSubscriptions);
    return newAgentState;
  });
}

} // namespace facebook::fboss
