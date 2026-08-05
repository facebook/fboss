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

#include "common/network/NetworkUtil.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/host/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <boost/algorithm/string.hpp>
#include <functional>
#include <string_view>

namespace facebook::fboss {

using facebook::network::NetworkUtil;
using utils::Table;

// Type alias for DNS resolver function: takes IP address string, returns
// hostname
using DnsResolver = std::function<std::string(const std::string&)>;

// Default DNS resolver using the real NetworkUtil
inline std::string defaultDnsResolver(const std::string& ipAddr) {
  return NetworkUtil::getHostByAddr(ipAddr);
}

struct CmdShowHostTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = utils::PortList;
  using RetType = cli::ShowHostModel;

  // Human-authored guide prose for the CLI reference wiki. Superset of the
  // one-line help string registered in the command tree.
  static std::string_view description();
};

class CmdShowHost : public CmdHandler<CmdShowHost, CmdShowHostTraits> {
 public:
  using ObjectArgType = CmdShowHostTraits::ObjectArgType;
  using RetType = CmdShowHostTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPorts,
      const DnsResolver& resolver = defaultDnsResolver);
  RetType createModel(
      const std::vector<NdpEntryThrift>& ndpEntries,
      const std::map<int32_t, PortInfoThrift>& portInfoEntries,
      const std::map<int32_t, PortStatus>& portStatusEntries,
      const ObjectArgType& queriedPorts,
      const DnsResolver& resolver = defaultDnsResolver);
  void printOutput(const RetType& model, std::ostream& out = std::cout);

  // Canned, synthetic model (no real switch data) used to render a
  // deterministic example for the CLI reference wiki. No live switch.
  static RetType sampleModel();
};

} // namespace facebook::fboss
