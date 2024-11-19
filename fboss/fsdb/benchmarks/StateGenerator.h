// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/agent_stats_types.h"

namespace facebook::fboss::fsdb::test {

class StateGenerator {
 public:
  static void fillVoqStats(
      AgentStats* stats,
      int nSysPorts,
      int nVoqsPerSysPort,
      int value = 1);
  static void updateVoqStats(AgentStats* stats, int increment = 1);
};

} // namespace facebook::fboss::fsdb::test
