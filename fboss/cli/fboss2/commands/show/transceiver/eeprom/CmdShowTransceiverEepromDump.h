// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/transceiver/eeprom/CmdShowTransceiverEeprom.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

#include <cstdint>
#include <string>
#include <vector>

namespace facebook::fboss {

struct CmdShowTransceiverEepromDumpTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowTransceiverEeprom;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = utils::PortList;
  using RetType = std::string;
};

class CmdShowTransceiverEepromDump : public CmdHandler<
                                         CmdShowTransceiverEepromDump,
                                         CmdShowTransceiverEepromDumpTraits> {
 public:
  using ObjectArgType = CmdShowTransceiverEepromDumpTraits::ObjectArgType;
  using RetType = CmdShowTransceiverEepromDumpTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::PortList& transceiverPorts,
      const utils::Message& eepromArgs,
      const ObjectArgType& queriedPorts);

  void printOutput(const RetType& output, std::ostream& out = std::cout);

 private:
  struct PageReadConfig {
    std::string name;
    int page; // -1 means don't set page
    int offset;
    int length;
  };

  static std::vector<PageReadConfig> getCmisPages();
  static std::vector<PageReadConfig> getSff8636Pages();
  static std::string
  formatHexDump(const uint8_t* data, int offset, size_t length);
};

} // namespace facebook::fboss
