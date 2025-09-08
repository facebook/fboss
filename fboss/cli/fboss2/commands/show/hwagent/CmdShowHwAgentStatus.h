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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/hwagent/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace facebook::fboss {

using utils::Table;

struct CmdShowHwAgentStatusTraits : public ReadCommandTraits {
  using RetType = cli::ShowHwAgentStatusModel;
};

struct SwHwAgentCounters {
  std::map<std::string, int64_t> FBSwCounters;
  std::vector<std::map<std::string, int64_t>> FBHwCountersVec;
};

/* Interface defined for mocking getAgentCounters in gtest*/
class AgentCountersIf {
 public:
  virtual ~AgentCountersIf() = default;
  virtual void getAgentCounters(HostInfo, int, SwHwAgentCounters&) = 0;
};

class AgentCounters : public AgentCountersIf {
 public:
  void getAgentCounters(
      HostInfo hostinfo,
      int numSwitches,
      SwHwAgentCounters& FBSwHwCounters) override;
};

class CmdShowHwAgentStatus
    : public CmdHandler<CmdShowHwAgentStatus, CmdShowHwAgentStatusTraits> {
  std::unique_ptr<AgentCounters> defaultCounters_;
  AgentCountersIf* counters_;

 public:
  using RetType = CmdShowHwAgentStatusTraits::RetType;
  CmdShowHwAgentStatus()
      : defaultCounters_(std::make_unique<AgentCounters>()),
        counters_(defaultCounters_.get()) {}
  CmdShowHwAgentStatus(AgentCountersIf* counters) : counters_(counters) {}
  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  int64_t getCounterValue(
      std::map<std::string, int64_t> counters,
      int switchIndex,
      const std::string& counterName);

  RetType createModel(
      std::map<int16_t, facebook::fboss::HwAgentEventSyncStatus>& hwAgentStatus,
      MultiSwitchRunState& runStates,
      struct SwHwAgentCounters FBSwHwCounters);

  std::string getRunStateStr(facebook::fboss::SwitchRunState runState);
};

} // namespace facebook::fboss
