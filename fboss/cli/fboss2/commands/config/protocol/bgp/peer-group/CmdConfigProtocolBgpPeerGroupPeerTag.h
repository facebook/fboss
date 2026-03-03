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
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroup.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdConfigProtocolBgpPeerGroupPeerTagTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpPeerGroup;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;
  using ObjectArgType = utils::Message;
  using RetType = std::string;
};

class CmdConfigProtocolBgpPeerGroupPeerTag
    : public CmdHandler<
          CmdConfigProtocolBgpPeerGroupPeerTag,
          CmdConfigProtocolBgpPeerGroupPeerTagTraits> {
 public:
  using ObjectArgType =
      CmdConfigProtocolBgpPeerGroupPeerTagTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpPeerGroupPeerTagTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::Message& groupName,
      const ObjectArgType& peerTag);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
