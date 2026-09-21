// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <ostream>
#include <string>
#include <variant>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/start/port/CmdStartPort.h"

namespace facebook::fboss {

struct CmdStartPortCableLengthMeasurementTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ParentCmd = CmdStartPort;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdStartPortCableLengthMeasurement
    : public CmdHandler<
          CmdStartPortCableLengthMeasurement,
          CmdStartPortCableLengthMeasurementTraits> {
 public:
  using RetType = CmdStartPortCableLengthMeasurementTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const utils::PortList& ports);
  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
