/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/dhcp/CmdDeleteDhcp.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/commands/config/dhcp/CmdConfigDhcp.h"

namespace facebook::fboss {

DhcpSourceOverrideDeleteArgs::DhcpSourceOverrideDeleteArgs(
    std::vector<std::string> v) {
  if (v.size() != 1) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <family>, got {} argument(s). Valid families: {}, {}",
            v.size(),
            DhcpSourceOverrideArgs::kFamilyIpv4,
            DhcpSourceOverrideArgs::kFamilyIpv6));
  }
  family_ = normalizeDhcpFamily(v[0]);
  v[0] = family_;
  data_ = std::move(v);
}

std::string removeDhcpSourceOverride(
    cfg::SwitchConfig& swConfig,
    std::string_view kind,
    const std::string& family) {
  auto field = dhcpSourceOverrideField(swConfig, kind, family);
  if (!field.has_value()) {
    throw std::invalid_argument(
        fmt::format("No dhcp {}-source-override {} configured", kind, family));
  }
  const std::string oldValue = *field;
  field.reset();
  return fmt::format(
      "Removed dhcp {}-source-override {} (was {})", kind, family, oldValue);
}

// Explicit template instantiation
template void CmdHandler<CmdDeleteDhcp, CmdDeleteDhcpTraits>::run();

} // namespace facebook::fboss
