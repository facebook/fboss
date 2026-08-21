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

#include <string>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/delete/interface/sflow/CmdDeleteInterfaceSflow.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

struct CmdDeleteInterfaceSflowSampleDestTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteInterfaceSflow;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

/*
 * Clears Port.sampleDest on the given port(s), returning the sFlow sample
 * destination to its unset default.
 */
class CmdDeleteInterfaceSflowSampleDest
    : public CmdHandler<
          CmdDeleteInterfaceSflowSampleDest,
          CmdDeleteInterfaceSflowSampleDestTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
