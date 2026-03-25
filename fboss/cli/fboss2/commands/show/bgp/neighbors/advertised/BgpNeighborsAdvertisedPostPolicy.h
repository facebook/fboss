// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/commands/show/bgp/neighbors/CmdShowBgpNeighbors.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "folly/String.h"

namespace facebook::fboss {
using neteng::fboss::bgp::thrift::TBgpPath;
using neteng::fboss::bgp_attr::TIpPrefix;

struct BgpNeighborsAdvertisedPostPolicyTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowBgpNeighbors;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = NetworkPathWithHost;
};

class BgpNeighborsAdvertisedPostPolicy
    : public CmdHandler<
          BgpNeighborsAdvertisedPostPolicy,
          BgpNeighborsAdvertisedPostPolicyTraits> {
 public:
  using RetType = BgpNeighborsAdvertisedPostPolicyTraits::RetType;
  using ObjectArgType = BgpNeighborsAdvertisedPostPolicy::ObjectArgType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIps,
      const ObjectArgType& prefixes) {
    if (queriedIps.empty()) {
      std::cout
          << "This command must have an IP address as argument: fboss2 show bgp neighbors [ADDR] post-policy [PREFIX]"
          << std::endl;
      return {};
    }
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
    std::map<TIpPrefix, std::vector<TBgpPath>> advertisedNetworks;
    client->sync_getPostfilterAdvertisedNetworks2(
        advertisedNetworks, queriedIps[0]);

    if (!prefixes.empty()) {
      advertisedNetworks = filterNetworks(advertisedNetworks, prefixes);
    }
    NetworkPathWithHost result;
    result.networkPath() = std::move(advertisedNetworks);
    result.host() = hostInfo.getName();
    result.oobName() = hostInfo.getOobName();
    result.ip() = hostInfo.getIpStr();
    return result;
  }

  void printOutput(
      const RetType& routesWithHost,
      std::ostream& out = std::cout) {
    const HostInfo hostInfo(
        *routesWithHost.host(),
        *routesWithHost.oobName(),
        folly::IPAddress(*routesWithHost.ip()));
    std::vector<std::string> output = printRoutesInformation(
        *routesWithHost.networkPath(), hostInfo, /*showPolicy=*/true);
    out << folly::join('\n', output) << std::endl;
  }
};

} // namespace facebook::fboss
