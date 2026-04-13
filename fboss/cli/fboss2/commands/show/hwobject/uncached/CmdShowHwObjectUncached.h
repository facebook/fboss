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

#include "fboss/cli/fboss2/commands/show/hwobject/CmdShowHwObject.h"

namespace facebook::fboss {

struct CmdShowHwObjectUncachedTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowHwObject;
  using RetType = std::string;
};

class CmdShowHwObjectUncached
    : public CmdHandler<CmdShowHwObjectUncached, CmdShowHwObjectUncachedTraits>,
      public CmdShowHwObjectTraits {
 public:
  using ObjectArgType = CmdShowHwObjectTraits::ObjectArgType;
  using RetType = CmdShowHwObjectUncachedTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedHwObjectTypes);

  void printOutput(const RetType& hwObjectInfo, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
