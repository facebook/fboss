// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h" // NOLINT(misc-include-cleaner)

namespace facebook::fboss {
using neteng::fboss::bgp::thrift::TRibEntry;
using neteng::fboss::bgp::thrift::TRibEntryWithHost;
using neteng::fboss::bgp_attr::TBgpAfi;

struct CmdShowBgpChangelistTraits : public ReadCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = TRibEntryWithHost;
};

class CmdShowBgpChangelist
    : public CmdHandler<CmdShowBgpChangelist, CmdShowBgpChangelistTraits> {
 public:
  using RetType = CmdShowBgpChangelistTraits::RetType;
  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(RetType& entries, std::ostream& out = std::cout);
};
} // namespace facebook::fboss
