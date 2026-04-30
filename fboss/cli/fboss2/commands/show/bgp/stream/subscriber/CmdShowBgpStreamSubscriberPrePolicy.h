// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/commands/show/bgp/stream/CmdShowBgpStreamSubscriber.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {
using namespace neteng::fboss::bgp::thrift;
using facebook::fboss::utils::Table;

struct CmdShowBgpStreamSubscriberPrePolicyTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowBgpStreamSubscriber;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = NetworkPathWithHost;
};

class CmdShowBgpStreamSubscriberPrePolicy
    : public CmdHandler<
          CmdShowBgpStreamSubscriberPrePolicy,
          CmdShowBgpStreamSubscriberPrePolicyTraits> {
 public:
  using RetType = CmdShowBgpStreamSubscriberPrePolicyTraits::RetType;
  using ObjectArgType = CmdShowBgpStreamSubscriberPrePolicy::ObjectArgType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& peerIds,
      const std::vector<std::string>& prefixes) {
    if (peerIds.empty()) {
      throw std::invalid_argument(
          "No subscriber ID provided. Hint: use `fboss2 show bgp stream summary` to see the subscribers.\n"
          "Usage: `fboss2 show bgp stream subscriber <subscriber id> pre-policy`\n"
          "Example: fboss2 show bgp stream subscriber 1 pre-policy");
    }

    std::map<TIpPrefix, std::vector<TBgpPath>> routes;
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
    client->sync_getSubscriberNetworkInfo(
        routes, folly::to<int32_t>(peerIds[0]), "pre-policy");

    if (!prefixes.empty()) {
      routes = filterNetworks(routes, prefixes);
    }
    NetworkPathWithHost result;
    result.networkPath() = std::move(routes);
    result.host() = hostInfo.getName();
    result.oobName() = hostInfo.getOobName();
    result.ip() = hostInfo.getIpStr();

    return result;
  }

  void printOutput(RetType& routesWithHost, std::ostream& out = std::cout) {
    const HostInfo hostInfo(
        *routesWithHost.host(),
        *routesWithHost.oobName(),
        folly::IPAddress(*routesWithHost.ip()));
    std::vector<std::string> output = printRoutesInformation(
        *routesWithHost.networkPath(), hostInfo, /*showPolicy=*/false);
    out << folly::join('\n', output) << std::endl;
  }
};

} // namespace facebook::fboss
