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
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/InterfacesConfig.h"

namespace facebook::fboss {

struct CmdConfigInterfaceQueuingPolicyTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;
  using ObjectArgType = utils::Message;
  using RetType = std::string;
};

class CmdConfigInterfaceQueuingPolicy
    : public CmdHandler<
          CmdConfigInterfaceQueuingPolicy,
          CmdConfigInterfaceQueuingPolicyTraits> {
 public:
  using ObjectArgType = CmdConfigInterfaceQueuingPolicyTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceQueuingPolicyTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfacesConfig& interfaceConfig,
      const ObjectArgType& policyName);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
