// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"

namespace facebook::fboss {
using facebook::neteng::fboss::bgp::thrift::TEntryStats;

struct CmdShowBgpStatsEntriesTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = TEntryStats;
};

class CmdShowBgpStatsEntries
    : public CmdHandler<CmdShowBgpStatsEntries, CmdShowBgpStatsEntriesTraits> {
 public:
  using RetType = CmdShowBgpStatsEntriesTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& stats, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
