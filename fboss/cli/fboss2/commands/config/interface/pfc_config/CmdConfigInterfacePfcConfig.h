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
#include "fboss/cli/fboss2/commands/config/interface/pfc_config/PfcConfigUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

using InterfaceList = utils::InterfaceList;

struct CmdConfigInterfacePfcConfigTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PFC_CONFIG_ATTRS;
  using ObjectArgType = utils::PfcConfigAttrs;
  using RetType = std::string;
};

class CmdConfigInterfacePfcConfig : public CmdHandler<
                                        CmdConfigInterfacePfcConfig,
                                        CmdConfigInterfacePfcConfigTraits> {
 public:
  using ObjectArgType = CmdConfigInterfacePfcConfigTraits::ObjectArgType;
  using RetType = CmdConfigInterfacePfcConfigTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const InterfaceList& interfaces,
      const ObjectArgType& config);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
