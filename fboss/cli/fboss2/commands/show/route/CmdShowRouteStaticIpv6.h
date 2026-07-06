/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteStatic.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowRouteStaticIpv6Traits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ParentCmd = CmdShowRouteStatic;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowRouteModel;
};

class CmdShowRouteStaticIpv6
    : public CmdHandler<CmdShowRouteStaticIpv6, CmdShowRouteStaticIpv6Traits> {
 public:
  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
