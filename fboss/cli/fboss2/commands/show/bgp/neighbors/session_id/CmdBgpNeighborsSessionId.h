// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h" // NOLINT(misc-include-cleaner)
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/commands/show/bgp/neighbors/CmdShowBgpNeighbors.h"

namespace facebook::fboss {

struct CmdBgpNeighborsSessionIdTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowBgpNeighbors;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PEERID_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = std::vector<TBgpSession>;
};

class CmdBgpNeighborsSessionId : public CmdHandler<
                                     CmdBgpNeighborsSessionId,
                                     CmdBgpNeighborsSessionIdTraits> {
 public:
  using RetType = CmdBgpNeighborsSessionIdTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedIps,
      const ObjectArgType& queriedSessions) {
    std::vector<TBgpSession> sessions;

    if (queriedSessions.empty()) {
      std::cout
          << "This command must have a session ID as argument: fboss2 show bgp neighbors [ADDR] session_id [Session ID]"
          << std::endl;
      return {};
    }

    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

    client->sync_getBgpNeighborsFromSession(
        sessions, queriedIps[0], queriedSessions[0]);

    return sessions;
  }

  void printOutput(const RetType& neighbors, std::ostream& out = std::cout) {
    printBgpNeighborsOutput(neighbors, out);
  }
};
} // namespace facebook::fboss
