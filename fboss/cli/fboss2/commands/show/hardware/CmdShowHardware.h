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
#include "fboss/cli/fboss2/commands/show/hardware/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "folly/dynamic.h"

namespace facebook::fboss {

using utils::Table;

/*
 Define the traits of this command. This will include the inputs and output
 types
*/
struct CmdShowHardwareTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowHardwareModel;
};

class CmdShowHardware
    : public CmdHandler<CmdShowHardware, CmdShowHardwareTraits> {
 public:
  using ObjectArgType = CmdShowHardwareTraits::ObjectArgType;
  using RetType = CmdShowHardwareTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  RetType createModel(folly::dynamic data, const std::string& product);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
