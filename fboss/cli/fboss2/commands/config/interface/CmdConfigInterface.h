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
#include "fboss/cli/fboss2/utils/InterfacesConfig.h"

namespace facebook::fboss {

struct CmdConfigInterfaceTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_INTERFACES_CONFIG;
  using ObjectArgType = utils::InterfacesConfig;
  using RetType = std::string;
};

class CmdConfigInterface
    : public CmdHandler<CmdConfigInterface, CmdConfigInterfaceTraits> {
 public:
  using ObjectArgType = CmdConfigInterfaceTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& interfaceConfig);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
