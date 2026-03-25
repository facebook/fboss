// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/routing/policy/if/gen-cpp2/policy_thrift_types.h"

namespace facebook::fboss {
using facebook::neteng::routing::policy::thrift::TPolicyStats;

struct CmdShowBgpStatsPolicyTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = TPolicyStats;
};

class CmdShowBgpStatsPolicy
    : public CmdHandler<CmdShowBgpStatsPolicy, CmdShowBgpStatsPolicyTraits> {
 public:
  using RetType = CmdShowBgpStatsPolicyTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& policy_stats, std::ostream& out = std::cout);
};
} // namespace facebook::fboss
