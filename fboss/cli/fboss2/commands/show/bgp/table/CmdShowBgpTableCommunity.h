// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/commands/show/bgp/table/CmdShowBgpTable.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {
using namespace neteng::fboss::bgp::thrift;

struct CmdShowBgpTableCommunityTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowBgpTable;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_COMMUNITY_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = TRibEntryWithHost;
};

class CmdShowBgpTableCommunity : public CmdHandler<
                                     CmdShowBgpTableCommunity,
                                     CmdShowBgpTableCommunityTraits> {
 public:
  using ObjectArgType = CmdShowBgpTableCommunityTraits::RetType;
  using RetType = CmdShowBgpTableCommunityTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& queriedCommunities) {
    std::vector<TRibEntry> entries;
    TRibEntryWithHost result;
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

    if (queriedCommunities.empty()) {
      std::cout
          << "No community entered. Usage: fboss2 show bgp table community <community>"
          << std::endl;
      return result;
    }

    std::vector<std::string> communityValues;
    try {
      communityValues = communityNameToValue(queriedCommunities[0], hostInfo);
    } catch (std::invalid_argument& e) {
      std::cout << e.what() << std::endl;
      return {};
    }

    client->sync_getRibEntriesForCommunity(
        entries, TBgpAfi::AFI_ALL, communityValues[0]);

    auto data = (communityValues.size() > 1)
        ? filterEntriesByCommunities(entries, communityValues[1])
        : entries;
    result.tRibEntries() = std::move(data);
    result.host() = hostInfo.getName();
    result.oobName() = hostInfo.getOobName();
    result.ip() = hostInfo.getIpStr();
    return result;
  }

  void printOutput(RetType& data, std::ostream& out = std::cout) {
    if (!data.tRibEntries()->empty()) {
      printRIBEntries(out, data, /*detail=*/true);
    }
  }

  std::vector<TRibEntry> filterEntriesByCommunities(
      std::vector<TRibEntry>& entries,
      const std::string& remainingCommunity) {
    for (auto& entry : entries) {
      auto& pathsMap = *entry.paths();
      for (auto it = pathsMap.begin(); it != pathsMap.end();) {
        const auto& [name, paths] = *it;
        std::vector<TBgpPath> filteredPaths;
        for (const auto& path : paths) {
          for (const auto& comm :
               *apache::thrift::get_pointer(path.communities())) {
            const std::string communityString = communityToString(comm);
            if (remainingCommunity == communityString) {
              filteredPaths.emplace_back(path);
            }
          }
        }
        if (filteredPaths.empty()) {
          it = pathsMap.erase(it); // erase returns iterator to next element
        } else {
          ++it;
        }
      }
    }
    std::vector<TRibEntry> result;
    for (const auto& entry : entries) {
      if (!entry.paths().value().empty()) {
        result.emplace_back(entry);
      }
    }
    return result;
  }
};
} // namespace facebook::fboss
