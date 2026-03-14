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
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowPortQueueTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowPort;
  // This command will inherit port list from its parent (ShowPort)
  // NoneArgType indicates that there is no args input
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = std::map<int32_t, facebook::fboss::PortInfoThrift>;
};

class CmdShowPortQueue
    : public CmdHandler<CmdShowPortQueue, CmdShowPortQueueTraits> {
 public:
  using ObjectArgType = CmdShowPortQueueTraits::ObjectArgType;
  using RetType = CmdShowPortQueueTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedPorts);

  void printOutput(
      const RetType& portId2PortInfoThrift,
      std::ostream& out = std::cout);
};

} // namespace facebook::fboss
