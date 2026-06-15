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

#include <fmt/format.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Argument for `config arp <attr> <value>`.
// Validates that v[0] is one of the valid ARP attribute names. The accepted
// shape of v[1] depends on the attribute:
//   - the timer attributes (timeout, age-interval, max-probes,
//     stale-interval) take a non-negative int32, exposed via getValue().
//   - the boolean toggle (proactive) takes "enabled" / "disabled", exposed
//     via getBoolValue().
class ArpConfigArgs : public utils::BaseObjectArgType<std::string> {
 public:
  enum class AttrKind { kInteger, kBoolean };

  /* implicit */ ArpConfigArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getAttribute() const {
    return attribute_;
  }

  // Valid only when attrKind_ == kInteger; throws std::logic_error otherwise.
  // For the boolean toggle use getBoolValue().
  int32_t getValue() const {
    if (attrKind_ == AttrKind::kBoolean) {
      throw std::logic_error(
          fmt::format(
              "getValue() called on boolean attr '{}'; use getBoolValue()",
              attribute_));
    }
    return value_;
  }

  // Valid only when attrKind_ == kBoolean; throws std::logic_error otherwise.
  // For integer attrs use getValue().
  bool getBoolValue() const {
    if (attrKind_ != AttrKind::kBoolean) {
      throw std::logic_error(
          fmt::format(
              "getBoolValue() called on non-boolean attr '{}'; use getValue()",
              attribute_));
    }
    return boolValue_;
  }

 private:
  std::string attribute_;
  AttrKind attrKind_{AttrKind::kInteger};
  int32_t value_ = 0;
  bool boolValue_ = false;
};

struct CmdConfigArpTraits : public WriteCommandTraits {
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "arp_attr_value",
        args,
        // Keep "enabled"/"disabled" in sync with
        // kProactiveEnabled/kProactiveDisabled in CmdConfigArp.cpp.
        "<attr> <value> where <attr> is one of: "
        "age-interval, max-probes, proactive, stale-interval, timeout. "
        "proactive takes enabled|disabled; all others take a non-negative integer.");
  }
  using ObjectArgType = ArpConfigArgs;
  using RetType = std::string;
};

class CmdConfigArp : public CmdHandler<CmdConfigArp, CmdConfigArpTraits> {
 public:
  using ObjectArgType = CmdConfigArpTraits::ObjectArgType;
  using RetType = CmdConfigArpTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& args);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
