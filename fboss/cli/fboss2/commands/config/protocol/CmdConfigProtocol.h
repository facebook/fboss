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

namespace facebook::fboss {

struct CmdConfigProtocolTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdConfigProtocol
    : public CmdHandler<CmdConfigProtocol, CmdConfigProtocolTraits> {
 public:
  using ObjectArgType = CmdConfigProtocolTraits::ObjectArgType;
  using RetType = CmdConfigProtocolTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
