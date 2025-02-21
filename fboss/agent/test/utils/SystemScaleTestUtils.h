#pragma once
#include "fboss/agent/test/AgentEnsemble.h"

namespace facebook::fboss::utility {

void initSystemScaleTest(AgentEnsemble* ensemble);
cfg::SwitchConfig getSystemScaleTestSwitchConfiguration(
    const AgentEnsemble& ensemble);

void configureMaxAclTable(AgentEnsemble* ensemble);
void configureMaxRouteTable(AgentEnsemble* ensemble);
void configureMaxMacTable(AgentEnsemble* ensemble);
void configureMaxNeighborTable(AgentEnsemble* ensemble);
} // namespace facebook::fboss::utility
