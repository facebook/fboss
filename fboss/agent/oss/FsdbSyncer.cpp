/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FsdbSyncer.h"

namespace facebook::fboss {

// oss definition
class AgentFsdbSyncManager {};

std::vector<std::string> FsdbSyncer::getAgentStatePath() {
  return {"agent"};
}

std::vector<std::string> FsdbSyncer::getAgentSwitchStatePath() {
  return {"agent", "switchState"};
}

std::vector<std::string> FsdbSyncer::getAgentSwitchConfigPath() {
  return {"agent", "config", "sw"};
}

std::vector<std::string> FsdbSyncer::getAgentStatsPath() {
  return {"agent"};
}

std::optional<std::string> FsdbSyncer::getBitsflowLockdownLevel() {
  return std::nullopt;
}

FsdbSyncer::FsdbSyncer(SwSwitch* /*sw*/) {}

FsdbSyncer::~FsdbSyncer() {}

void FsdbSyncer::stop() {}

void FsdbSyncer::stateUpdated(const StateDelta& /*stateDelta*/) {}

void FsdbSyncer::cfgUpdated(
    const cfg::SwitchConfig& /*oldConfig*/,
    const cfg::SwitchConfig& /*newConfig*/) {}

void FsdbSyncer::statsUpdated(const AgentStats& /*stats*/) {}

void FsdbSyncer::fsdbStatPublisherStateChanged(
    fsdb::FsdbStreamClient::State /*oldState*/,
    fsdb::FsdbStreamClient::State /*newState*/) {}

void FsdbSyncer::updateDsfSubscriberState(
    const std::string& /*nodeName*/,
    fsdb::FsdbSubscriptionState /*oldState*/,
    fsdb::FsdbSubscriptionState /*newState*/) {}

} // namespace facebook::fboss
