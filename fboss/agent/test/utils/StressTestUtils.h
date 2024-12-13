#pragma once
#include "fboss/agent/test/AgentEnsemble.h"

namespace facebook::fboss::utility {
template <typename RouteScaleGeneratorT>
void resolveNhopForRouteGenerator(AgentEnsemble* ensemble);

template <typename RouteScaleGeneratorT>
std::tuple<double, double> routeChangeLookupStresser(AgentEnsemble* ensemble);

cfg::SwitchConfig bgpRxBenchmarkConfig(const AgentEnsemble& ensemble);

} // namespace facebook::fboss::utility
