/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

namespace facebook::fboss {
using facebook::neteng::fboss::bgp::thrift::THealthReport;

struct CmdShowBgpHealthTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = THealthReport;
};

class CmdShowBgpHealth
    : public CmdHandler<CmdShowBgpHealth, CmdShowBgpHealthTraits> {
 public:
  using RetType = CmdShowBgpHealthTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& report, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
