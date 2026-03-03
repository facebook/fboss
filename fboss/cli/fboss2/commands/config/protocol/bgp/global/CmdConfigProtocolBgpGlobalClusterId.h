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
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobal.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdConfigProtocolBgpGlobalClusterIdTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigProtocolBgpGlobal;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ObjectArgType = utils::IPList;
  using RetType = std::string;
};

class CmdConfigProtocolBgpGlobalClusterId
    : public CmdHandler<
          CmdConfigProtocolBgpGlobalClusterId,
          CmdConfigProtocolBgpGlobalClusterIdTraits> {
 public:
  using ObjectArgType =
      CmdConfigProtocolBgpGlobalClusterIdTraits::ObjectArgType;
  using RetType = CmdConfigProtocolBgpGlobalClusterIdTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& clusterId);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
