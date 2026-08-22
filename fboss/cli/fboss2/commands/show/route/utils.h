// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"

namespace facebook::fboss::show::route::utils {

bool isFpfEncoding(
    const std::optional<facebook::bgp::nsf_policy::NsfTeWeightEncoding>&
        encoding);

// CLI-facing protocol name for a route client ("bgp", "static", ...).
std::string getProtocolStr(ClientID clientId);

// The client whose entry the RIB prefers for this route, derived from the
// default client -> admin-distance ranking (RouteDetails does not export the
// RIB's lowestAdminDistanceClientId). Routes with reconfigured admin
// distances and multiple clients on one prefix may be mislabeled.
ClientID getBestClientId(const facebook::fboss::RouteDetails& entry);

// "ipv4" or "ipv6", from the route's destination prefix.
std::string getAddressFamilyStr(const facebook::fboss::RouteDetails& entry);

std::string getMplsActionCodeStr(MplsActionCode mplsActionCode);

std::string getMplsActionInfoStr(const cli::MplsActionInfo& mplsActionInfo);

void getNextHopInfoAddr(
    const network::thrift::BinaryAddress& addr,
    cli::NextHopInfo& nextHopInfo);

void getNextHopInfoThrift(
    const NextHopThrift& nextHop,
    cli::NextHopInfo& nextHopInfo);

std::string getNextHopInfoStr(
    const cli::NextHopInfo& nextHopInfo,
    const std::optional<facebook::bgp::nsf_policy::NsfTeWeightEncoding>&
        encoding = std::nullopt);
std::string getNextHopInfoStr(
    const cli::NextHopInfo& nextHopInfo,
    const std::map<std::string, std::string>& vlanAggregatePortMap,
    const std::map<
        std::string,
        std::map<std::string, std::vector<std::string>>>& vlanPortMap,
    const std::optional<facebook::bgp::nsf_policy::NsfTeWeightEncoding>&
        encoding = std::nullopt);

} // namespace facebook::fboss::show::route::utils
