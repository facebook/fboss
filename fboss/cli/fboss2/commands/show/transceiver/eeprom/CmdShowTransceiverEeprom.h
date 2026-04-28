// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/transceiver/CmdShowTransceiver.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

#include <cstdint>
#include <string>
#include <vector>

namespace facebook::fboss {

struct CmdShowTransceiverEepromTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowTransceiver;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE;
  using ObjectArgType = utils::Message; // page, offset, length
  using RetType = std::string;
};

class CmdShowTransceiverEeprom : public CmdHandler<
                                     CmdShowTransceiverEeprom,
                                     CmdShowTransceiverEepromTraits> {
 public:
  using ObjectArgType = CmdShowTransceiverEepromTraits::ObjectArgType;
  using RetType = CmdShowTransceiverEepromTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& queriedPorts,
      const ObjectArgType& args);

  void printOutput(const RetType& output, std::ostream& out = std::cout);

 private:
  static std::string
  formatHexDump(const uint8_t* data, int offset, size_t length);
};

} // namespace facebook::fboss
