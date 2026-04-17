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

#include <cstdint>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Argument for `config arp <attr> <value>`.
// Validates that v[0] is one of the valid ARP attribute names and v[1] is a
// non-negative int32.
class ArpConfigArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ ArpConfigArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& getAttribute() const {
    return attribute_;
  }

  int32_t getValue() const {
    return value_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ARP_CONFIG;

 private:
  std::string attribute_;
  int32_t value_ = 0;
};

struct CmdConfigArpTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ARP_CONFIG;
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
