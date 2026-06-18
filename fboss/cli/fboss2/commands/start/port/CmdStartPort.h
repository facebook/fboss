// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdStartPortTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = utils::PortList;
  using RetType = std::string;
};

class CmdStartPort : public CmdHandler<CmdStartPort, CmdStartPortTraits> {
 public:
  using ObjectArgType = CmdStartPortTraits::ObjectArgType;
  using RetType = CmdStartPortTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& ports);
  void printOutput(const RetType& model);
};

} // namespace facebook::fboss
