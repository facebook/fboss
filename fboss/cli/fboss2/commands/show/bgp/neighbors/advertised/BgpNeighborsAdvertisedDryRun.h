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

namespace facebook::fboss {
using neteng::fboss::bgp::thrift::TBgpPath;
using neteng::fboss::bgp_attr::TIpPrefix;

struct BgpNeighborsAdvertisedDryRunTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowBgpNeighbors;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = NetworkPathWithHost;
};

class BgpNeighborsAdvertisedDryRun : public CmdHandler<
                                         BgpNeighborsAdvertisedDryRun,
                                         BgpNeighborsAdvertisedDryRunTraits> {
 public:
  using RetType = BgpNeighborsAdvertisedDryRunTraits::RetType;
  using ObjectArgType = BgpNeighborsAdvertisedDryRun::ObjectArgType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedIps,
      const ObjectArgType& prefixes) {
    if (queriedIps.empty()) {
      std::cout
          << "This command must have an IP address as argument: fboss2 show bgp neighbors [ADDR] dryrun-post-policy [PREFIX]"
          << std::endl;
      return {};
    }
    const std::string kBgpDryRunPath = "/var/tmp/bgpcpp.conf";
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

    std::cout << "Attempting to read file from " << kBgpDryRunPath << "\n"
              << std::endl;

    std::map<TIpPrefix, TBgpPath> networks;
    client->sync_getDryRunPostfilterAdvertisedNetworks(
        networks, queriedIps[0], kBgpDryRunPath);

    NetworkPathWithHost result;
    result.networkPath() = vectorizePaths(networks);
    if (!prefixes.empty()) {
      result.networkPath() = filterNetworks(*result.networkPath(), prefixes);
    }
    result.host() = hostInfo.getName();
    result.oobName() = hostInfo.getOobName();
    result.ip() = hostInfo.getIpStr();
    return result;
  }

  void printOutput(
      const RetType& networksWithHost,
      std::ostream& out = std::cout) {
    const HostInfo hostInfo(
        *networksWithHost.host(),
        *networksWithHost.oobName(),
        folly::IPAddress(*networksWithHost.ip()));
    std::vector<std::string> output = printRoutesInformation(
        *networksWithHost.networkPath(), hostInfo, /*showPolicy=*/true);
    out << folly::join('\n', output) << std::endl;
  }
};

} // namespace facebook::fboss
