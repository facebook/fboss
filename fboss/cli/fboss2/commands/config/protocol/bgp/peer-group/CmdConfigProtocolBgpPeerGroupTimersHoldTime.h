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
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimers.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdConfigProtocolBgpPeerGroupTimersHoldTimeTraits
    : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpPeerGroupTimers;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;
  using ObjectArgType = utils::Message;
  using RetType = std::string;
};

class CmdConfigProtocolBgpPeerGroupTimersHoldTime
    : public CmdHandler<
          CmdConfigProtocolBgpPeerGroupTimersHoldTime,
          CmdConfigProtocolBgpPeerGroupTimersHoldTimeTraits> {
 public:
  using ObjectArgType =
      CmdConfigProtocolBgpPeerGroupTimersHoldTimeTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpPeerGroupTimersHoldTimeTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::Message& peerGroupName,
      const ObjectArgType& holdTime);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
