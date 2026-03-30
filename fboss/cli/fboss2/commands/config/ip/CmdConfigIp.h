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

struct CmdConfigIpTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdConfigIp : public CmdHandler<CmdConfigIp, CmdConfigIpTraits> {
 public:
  using ObjectArgType = CmdConfigIpTraits::ObjectArgType;
  using RetType = CmdConfigIpTraits::RetType;

  RetType queryClient(const HostInfo& /* hostInfo */);

  void printOutput(const RetType& /* model */);
};

// IPv6 parent command - shares same structure as IPv4
struct CmdConfigIpv6Traits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdConfigIpv6 : public CmdHandler<CmdConfigIpv6, CmdConfigIpv6Traits> {
 public:
  using ObjectArgType = CmdConfigIpv6Traits::ObjectArgType;
  using RetType = CmdConfigIpv6Traits::RetType;

  RetType queryClient(const HostInfo& /* hostInfo */);

  void printOutput(const RetType& /* model */);
};

} // namespace facebook::fboss
