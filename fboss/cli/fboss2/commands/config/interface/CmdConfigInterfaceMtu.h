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
#include <folly/Conv.h>
#include <folly/String.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

// Custom type for MTU argument with validation
class MtuValue : public utils::BaseObjectArgType<int32_t> {
 public:
  /* implicit */ MtuValue(std::vector<std::string> v) {
    if (v.empty()) {
      throw std::invalid_argument("MTU value is required");
    }
    if (v.size() != 1) {
      throw std::invalid_argument(
          "Expected single MTU value, got: " + folly::join(", ", v));
    }

    try {
      int32_t mtu = folly::to<int32_t>(v[0]);
      if (mtu < utils::kMtuMin || mtu > utils::kMtuMax) {
        throw std::invalid_argument(
            fmt::format(
                "MTU must be between {} and {} inclusive, got: {}",
                utils::kMtuMin,
                utils::kMtuMax,
                mtu));
      }
      data_.push_back(mtu);
    } catch (const folly::ConversionError&) {
      throw std::invalid_argument("Invalid MTU value: " + v[0]);
    }
  }

  int32_t getMtu() const {
    return data_[0];
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_MTU;
};

struct CmdConfigInterfaceMtuTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterface;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_MTU;
  using ObjectArgType = MtuValue;
  using RetType = std::string;
};

class CmdConfigInterfaceMtu
    : public CmdHandler<CmdConfigInterfaceMtu, CmdConfigInterfaceMtuTraits> {
 public:
  using ObjectArgType = CmdConfigInterfaceMtuTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceMtuTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& mtu);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
