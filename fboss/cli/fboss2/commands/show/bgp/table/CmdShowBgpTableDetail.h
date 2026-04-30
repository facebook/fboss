// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {
using namespace neteng::fboss::bgp::thrift;
using namespace neteng::fboss::bgp_attr;

struct CmdShowBgpTableDetailTraits : public ReadCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = TRibEntryWithHost;
};

class CmdShowBgpTableDetail
    : public CmdHandler<CmdShowBgpTableDetail, CmdShowBgpTableDetailTraits> {
 public:
  using RetType = CmdShowBgpTableDetailTraits::RetType;
  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<TRibEntry> entriesIPv4;
    std::vector<TRibEntry> entriesIPv6;
    auto client = utils::createClient<apache::thrift::Client<
        facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

    client->sync_getRibEntries(entriesIPv4, TBgpAfi::AFI_IPV4);
    client->sync_getRibEntries(entriesIPv6, TBgpAfi::AFI_IPV6);

    // Append both entries into a single vector
    entriesIPv4.insert(
        entriesIPv4.end(), entriesIPv6.begin(), entriesIPv6.end());
    TRibEntryWithHost result;
    result.tRibEntries() = std::move(entriesIPv4);
    result.host() = hostInfo.getName();
    result.oobName() = hostInfo.getOobName();
    result.ip() = hostInfo.getIpStr();
    return result;
  }

  void printOutput(RetType& entries, std::ostream& out = std::cout) {
    printRIBEntries(out, entries, /*detail=*/true);
  }
};
} // namespace facebook::fboss
