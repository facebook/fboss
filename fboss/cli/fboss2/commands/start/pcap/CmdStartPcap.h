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
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdStartPcapTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
  std::vector<utils::LocalOption> LocalOptions = {
      {"--name", "Name to identify packet capture"},
      {"--max", "Max number of packets to capture"},
      {"--direction", "Direction of packets (txonly, rxonly, txrx)"}};
};

class CmdStartPcap : public CmdHandler<CmdStartPcap, CmdStartPcapTraits> {
 public:
  using ObjectArgType = CmdStartPcapTraits::ObjectArgType;
  using RetType = CmdStartPcapTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& captureOutput, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
