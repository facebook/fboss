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

struct CmdShowBgpTablePrefixTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowBgpTable;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = TRibEntryWithHost;
};

class CmdShowBgpTablePrefix
    : public CmdHandler<CmdShowBgpTablePrefix, CmdShowBgpTablePrefixTraits> {
 public:
  using ObjectArgType = CmdShowBgpTablePrefixTraits::RetType;
  using RetType = CmdShowBgpTablePrefixTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const std::vector<std::string>& prefixes) {
    std::vector<TRibEntry> allEntries;
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
    TRibEntryWithHost result;

    if (prefixes.empty()) {
      std::cout
          << "No prefixes entered. Usage: fboss2 show bgp table prefix <prefix>"
          << std::endl;
      return result;
    }

    for (const auto& prefix : prefixes) {
      std::vector<TRibEntry> newEntry;
      client->sync_getRibPrefix(newEntry, prefix);
      allEntries.insert(allEntries.end(), newEntry.begin(), newEntry.end());
    }
    result.tRibEntries() = std::move(allEntries);
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
};
} // namespace facebook::fboss
