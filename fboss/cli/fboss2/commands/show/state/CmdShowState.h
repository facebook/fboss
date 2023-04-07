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
#include "fboss/cli/fboss2/commands/show/state/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/commands/show/state/gen-cpp2/model_visitation.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowStateTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowStateModel;
};

class CmdShowState : public CmdHandler<CmdShowState, CmdShowStateTraits> {
 public:
  RetType queryClient(const HostInfo& hostInfo) {
    // TBD: Query FBOSS local drainer for state

    RetType model{};
    model.switchState() = "Unsupported (TBD)";
    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    out << model.get_switchState() << std::endl;
  }
};

} // namespace facebook::fboss
