// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

#include <string>

namespace facebook::fboss {

struct CmdShowTransceiverLoopbackTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowTransceiver;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdShowTransceiverLoopback : public CmdHandler<
                                       CmdShowTransceiverLoopback,
                                       CmdShowTransceiverLoopbackTraits> {
 public:
  using RetType = CmdShowTransceiverLoopbackTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedPorts);

  void printOutput(const RetType& output, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
