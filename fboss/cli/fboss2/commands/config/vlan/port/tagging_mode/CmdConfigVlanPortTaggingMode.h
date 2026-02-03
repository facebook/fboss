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

#include <folly/String.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/vlan/port/CmdConfigVlanPort.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

// Custom type for tagging mode argument with validation
class TaggingModeArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ TaggingModeArg( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v) {
    if (v.empty()) {
      throw std::invalid_argument(
          "Tagging mode is required (tagged or untagged)");
    }
    if (v.size() != 1) {
      throw std::invalid_argument(
          "Expected exactly one tagging mode (tagged or untagged)");
    }

    std::string mode = v[0];
    folly::toLowerAscii(mode);
    if (mode == "tagged") {
      emitTags_ = true;
    } else if (mode == "untagged") {
      emitTags_ = false;
    } else {
      throw std::invalid_argument(
          "Invalid tagging mode '" + v[0] +
          "'. Expected 'tagged' or 'untagged'");
    }
    data_.push_back(v[0]);
  }

  bool getEmitTags() const {
    return emitTags_;
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PORT_AND_TAGGING_MODE;

 private:
  bool emitTags_ = false;
};

struct CmdConfigVlanPortTaggingModeTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigVlanPort;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_PORT_AND_TAGGING_MODE;
  using ObjectArgType = TaggingModeArg;
  using RetType = std::string;
};

class CmdConfigVlanPortTaggingMode : public CmdHandler<
                                         CmdConfigVlanPortTaggingMode,
                                         CmdConfigVlanPortTaggingModeTraits> {
 public:
  using ObjectArgType = CmdConfigVlanPortTaggingModeTraits::ObjectArgType;
  using RetType = CmdConfigVlanPortTaggingModeTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const VlanId& vlanId,
      const utils::PortList& portList,
      const ObjectArgType& taggingMode);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
