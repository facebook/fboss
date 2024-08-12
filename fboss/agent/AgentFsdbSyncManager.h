// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbSyncManager.h"
#ifndef IS_OSS
#include "configerator/structs/neteng/fboss/bitsflow/gen-cpp2/bitsflow_types.h"
#endif
#include "fboss/fsdb/if/FsdbModel.h"

#include "fboss/agent/gen-cpp2/switch_reachability_types.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

DECLARE_bool(fsdb_sync_full_state);
DECLARE_bool(agent_fsdb_sync);

namespace facebook::fboss {

using fsdb_model_tags = fsdb::fsdb_model_tags::strings;
using agent_config_tags = cfg::agent_config_tags::strings;
#ifndef IS_OSS
namespace cfgr_bitsflow = ::configerator::structs::neteng::fboss::bitsflow;
#endif

template <>
struct thrift_cow::ResolveMemberType<
    thrift_cow::ThriftStructNode<fsdb::AgentData>,
    fsdb_model_tags::switchState> : std::true_type {
  using type = SwitchState;
};

class AgentFsdbSyncManager
    : public fsdb::
          FsdbSyncManager<fsdb::AgentData, true /* EnablePatchAPIs */> {
  /* list of maps which are subscribed to by external consumers */
  using SubscribedMaps = std::tuple<
      switch_state_tags::portMaps,
      switch_state_tags::transceiverMaps,
      switch_state_tags::qosPolicyMaps,
      switch_state_tags::sflowCollectorMaps,
      switch_state_tags::aggregatePortMaps,
      switch_state_tags::systemPortMaps,
      switch_state_tags::vlanMaps,
      switch_state_tags::interfaceMaps>;

 public:
  using Base = fsdb::FsdbSyncManager<fsdb::AgentData>;
  using AgentData = Base::CowState;
  using AgentDataSwitchState = typename AgentData::Fields::TypeFor<
      fsdb_model_tags::switchState>::element_type;

  static_assert(
      std::is_same_v<AgentDataSwitchState, SwitchState>,
      "Switch state not represented in agent data");

  explicit AgentFsdbSyncManager(
      const std::shared_ptr<fsdb::FsdbPubSubManager>& pubSubMgr);
  AgentFsdbSyncManager();

  // invoke with a new state to send
  void stateUpdated(const StateDelta& delta);
  void cfgUpdated(
      const cfg::SwitchConfig& oldConfig,
      const cfg::SwitchConfig& newConfig);
#ifndef IS_OSS
  void bitsflowLockdownLevelUpdated(
      std::optional<cfgr_bitsflow::BitsflowLockdownLevel> oldLevel,
      std::optional<cfgr_bitsflow::BitsflowLockdownLevel> newLevel);
#endif
  void updateDsfSubscriberState(
      const std::string& nodeName,
      fsdb::FsdbSubscriptionState oldState,
      fsdb::FsdbSubscriptionState newState);
  void switchReachabilityChanged(
      int64_t switchId,
      switch_reachability::SwitchReachability newReachability);

 private:
  void stateUpdatedDelta(const StateDelta& delta);

  static bool modify(
      std::shared_ptr<facebook::fboss::SwitchState>& oldState,
      const std::shared_ptr<facebook::fboss::SwitchState>& newState);
};

} // namespace facebook::fboss
