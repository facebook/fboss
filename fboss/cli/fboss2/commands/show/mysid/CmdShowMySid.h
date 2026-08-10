// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string_view>
#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/mysid/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

namespace facebook::fboss {

struct CmdShowMySidTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowMySidModel;

  // Human-authored guide prose for the CLI reference wiki. Superset of the
  // one-line help string registered in the command tree.
  static std::string_view description();
};

class CmdShowMySid : public CmdHandler<CmdShowMySid, CmdShowMySidTraits> {
 public:
  using RetType = CmdShowMySidTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  RetType createModel(
      const std::vector<MySidEntry>& entries,
      const std::vector<ArpEntryThrift>& arpTable,
      const std::vector<NdpEntryThrift>& ndpTable,
      const std::map<int32_t, PortInfoThrift>& portInfo,
      const std::vector<AggregatePortThrift>& aggregatePorts);
  void printOutput(const RetType& model, std::ostream& out = std::cout);

  // Canned, synthetic model (no real switch data) used to render a
  // deterministic example for the CLI reference wiki. No live switch.
  static RetType sampleModel();
};

} // namespace facebook::fboss
