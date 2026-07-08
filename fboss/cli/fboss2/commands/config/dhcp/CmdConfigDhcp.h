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

#include <string>
#include <string_view>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss::cfg {
class SwitchConfig;
} // namespace facebook::fboss::cfg

namespace facebook::fboss {

// Shared argument type for `config dhcp {relay,reply}-source-override <family>
// <ip>`. Validates family ∈ {"ipv4","ipv6"} and the IP via
// folly::IPAddressV{4,6}::tryFromString.
class DhcpSourceOverrideArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ DhcpSourceOverrideArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getFamily() const {
    return family_;
  }

  const std::string& getIpAddress() const {
    return ipAddress_;
  }

  static constexpr std::string_view kFamilyIpv4 = "ipv4";
  static constexpr std::string_view kFamilyIpv6 = "ipv6";

 private:
  std::string family_;
  std::string ipAddress_;
};

// Mutates swConfig for the given (kind, family, ip) triple and returns the
// human-readable result string. kind must be "relay" or "reply";
// family must already be normalised by DhcpSourceOverrideArgs.
std::string applyDhcpSourceOverride(
    cfg::SwitchConfig& swConfig,
    std::string_view kind,
    const std::string& family,
    const std::string& ipAddress);

struct CmdConfigDhcpTraits : public WriteCommandTraits {
  using ObjectArgType = utils::NoneArgType;
  using RetType = std::string;
};

class CmdConfigDhcp : public CmdHandler<CmdConfigDhcp, CmdConfigDhcpTraits> {
 public:
  using ObjectArgType = CmdConfigDhcpTraits::ObjectArgType;
  using RetType = CmdConfigDhcpTraits::RetType;

  RetType queryClient(const HostInfo& /* hostInfo */) {
    throw std::runtime_error(
        "Incomplete command, please use 'relay-source-override' or "
        "'reply-source-override' subcommand");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
