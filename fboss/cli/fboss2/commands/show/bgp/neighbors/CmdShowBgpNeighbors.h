// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

using facebook::neteng::fboss::bgp::thrift::TBgpSession;

struct CmdShowBgpNeighborsTraits : public ReadCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::vector<TBgpSession>;

  std::vector<utils::LocalOption> LocalOptions = {
      {kGar, "Show decoded GAR link bandwidth ext community"}};
};

class CmdShowBgpNeighbors
    : public CmdHandler<CmdShowBgpNeighbors, CmdShowBgpNeighborsTraits> {
 public:
  using RetType = CmdShowBgpNeighborsTraits::RetType;
  // - If no arguments are passed, the command will query all the neighbors
  //
  // - The first argument will always be considered as the neighbor ip
  // - The second argument will always be consdiered as the session id
  // - Every other address after the session id will be ignored.
  //
  // Note: Currently '--session_id' flag is no supported on FBOSS2.
  // In the mean time, the way to query over a neighbor with a session id,
  // the id must be passed right after the peer IP like the following:
  //    fboss2 show bgp neighbors 1.2.3.4 5.6.7.8
  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedIps) {
    std::vector<TBgpSession> sessions;

    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

    if (queriedIps.size() > 1) {
      std::cout << "Displaying information from neighbor: " << queriedIps[0]
                << ", session id: " << queriedIps[1] << std::endl;
      client->sync_getBgpNeighborsFromSession(
          sessions, /*p_peerId=*/queriedIps[0], /*sessionBgpId=*/queriedIps[1]);
    } else {
      client->sync_getBgpNeighbors(sessions, queriedIps);
    }

    return sessions;
  }

  void printOutput(const RetType& neighbors, std::ostream& out = std::cout) {
    printBgpNeighborsOutput(neighbors, out);
  }
};
} // namespace facebook::fboss
