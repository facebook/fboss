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
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeer.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdConfigProtocolBgpPeerTimersTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpPeer;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdConfigProtocolBgpPeerTimers
    : public CmdHandler<
          CmdConfigProtocolBgpPeerTimers,
          CmdConfigProtocolBgpPeerTimersTraits> {
 public:
  using ObjectArgType = CmdConfigProtocolBgpPeerTimersTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpPeerTimersTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::IPList& peerAddress);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
