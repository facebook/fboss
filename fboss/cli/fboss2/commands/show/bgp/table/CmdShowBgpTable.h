// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {
using namespace neteng::fboss::bgp::thrift;
using namespace neteng::fboss::bgp_attr;

struct CmdShowBgpTableTraits : public ReadCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = TRibEntryWithHost;

  std::vector<utils::LocalOption> LocalOptions = {
      {kGar, "Show decoded GAR link bandwidth ext community"}};
};

class CmdShowBgpTable
    : public CmdHandler<CmdShowBgpTable, CmdShowBgpTableTraits> {
 public:
  using RetType = CmdShowBgpTableTraits::RetType;
  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(RetType& entries, std::ostream& out = std::cout);
};
} // namespace facebook::fboss
