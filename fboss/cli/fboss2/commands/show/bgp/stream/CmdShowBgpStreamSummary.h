// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h" // NOLINT(misc-include-cleaner)
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {
using namespace neteng::fboss::bgp::thrift;
using facebook::fboss::utils::Table;

struct CmdShowBgpStreamSummaryTraits : public ReadCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::vector<TBgpStreamSession>;
};

class CmdShowBgpStreamSummary : public CmdHandler<
                                    CmdShowBgpStreamSummary,
                                    CmdShowBgpStreamSummaryTraits> {
 public:
  using RetType = CmdShowBgpStreamSummaryTraits::RetType;
  using ObjectArgType = CmdShowBgpStreamSummary::ObjectArgType;

  RetType queryClient(const HostInfo& hostInfo) {
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
    std::vector<TBgpStreamSession> sessions;
    client->sync_getBgpStreamSessions(sessions);

    return sessions;
  }

  void printOutput(RetType& sessions, std::ostream& out = std::cout) {
    if (sessions.empty()) {
      return;
    }

    std::sort(
        sessions.begin(),
        sessions.end(),
        [](const TBgpStreamSession& a, const TBgpStreamSession& b) -> bool {
          return folly::copy(a.peer_id().value()) <
              folly::copy(b.peer_id().value());
        });

    out << "BGP stream summary information for subscribers\n" << std::endl;
    Table table;
    table.setHeader({
        "Peer ID",
        "Name",
        "NumRoutes",
        "Uptime",
    });

    for (const auto& session : sessions) {
      table.addRow(
          {folly::to<std::string>(folly::copy(session.peer_id().value())),
           session.subscriber_name().value(),
           folly::to<std::string>(*session.sent_prefix_count()),
           utils::getPrettyElapsedTime(
               utils::getEpochFromDuration(
                   folly::copy(session.uptime().value())))});
    }
    out << table << std::endl;
  }
};

} // namespace facebook::fboss
