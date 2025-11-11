/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/test/agent_multinode_tests/TopologyInfo.h"

namespace facebook::fboss::utility {

void verifyDsfCluster(const std::unique_ptr<TopologyInfo>& topologyInfo);

bool verifyDsfGracefulAgentRestart(
    const std::unique_ptr<TopologyInfo>& topologyInfo);
bool verifyDsfUngracefulAgentRestart(
    const std::unique_ptr<TopologyInfo>& topologyInfo);
bool verifyDsfGracefulAgentRestartTimeoutRecovery(
    const std::unique_ptr<TopologyInfo>& topologyInfo);

bool verifyDsfGracefulQsfpRestart(
    const std::unique_ptr<TopologyInfo>& topologyInfo);
bool verifyDsfUngracefulQsfpRestart(
    const std::unique_ptr<TopologyInfo>& topologyInfo);

bool verifyDsfGracefulFSDBRestart(
    const std::unique_ptr<TopologyInfo>& topologyInfo);
bool verifyDsfUngracefulFSDBRestart(
    const std::unique_ptr<TopologyInfo>& topologyInfo);

bool verifyDsfGracefulFabricLinkDownUp(
    const std::unique_ptr<TopologyInfo>& topologyInfo);

bool verifyDsfFabricLinkDrainUndrain(
    const std::unique_ptr<TopologyInfo>& topologyInfo);

bool verifyFabricSpray(const std::string& rdsw);

std::map<std::string, std::map<std::string, DsfSessionThrift>>
getPeerToDsfSessionForRdsws(const std::set<std::string>& rdsws);

bool verifyNoSessionsFlapForRdsws(
    const std::set<std::string>& rdsws,
    std::map<std::string, std::map<std::string, DsfSessionThrift>>&
        baselineRdswToPeerAndDsfSession);

bool verifyNoReassemblyErrorsForAllSwitches(
    const std::unique_ptr<utility::TopologyInfo>& topologyInfo);

std::set<std::string> getOneFabricSwitchForEachCluster(
    const std::unique_ptr<utility::TopologyInfo>& topologyInfo);

int32_t getFirstActiveFabricPort(const std::string& switchName);

} // namespace facebook::fboss::utility
