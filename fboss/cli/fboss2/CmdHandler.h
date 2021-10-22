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

#include <CLI/CLI.hpp>

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

#include "fboss/cli/fboss2/CmdArgsLists.h"
#include "fboss/cli/fboss2/CmdSubcommands.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

#include <fmt/color.h>
#include <fmt/format.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

template <typename CmdTypeT, typename CmdTypeTraits>
class CmdHandler {
 public:
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      CmdTypeTraits::ObjectArgTypeId;
  using ObjectArgType = typename CmdTypeTraits::ObjectArgType;
  using RetType = typename CmdTypeTraits::RetType;

  void run();

 private:
  CmdTypeT& impl() {
    return static_cast<CmdTypeT&>(*this);
  }
  const CmdTypeT& impl() const {
    return static_cast<const CmdTypeT&>(*this);
  }

  RetType queryClientHelper(const HostInfo& hostInfo) {
    RetType result;
    if constexpr (
        ObjectArgTypeId == utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE) {
      result = impl().queryClient(hostInfo);
    } else {
      result = impl().queryClient(hostInfo, CmdArgsLists::getInstance()->at(0));
    }

    return result;
  }

  std::tuple<std::string, RetType, std::string> asyncHandler(
      const std::string& host) {
    auto hostInfo = HostInfo(host);
    XLOG(DBG2) << "host: " << host << " ip: " << hostInfo.getIpStr();

    std::string errStr;
    RetType result;
    try {
      result = queryClientHelper(hostInfo);
    } catch (std::exception const& err) {
      errStr = folly::to<std::string>("Thrift call failed: '", err.what(), "'");
    }

    return std::make_tuple(host, result, errStr);
  }
};

} // namespace facebook::fboss
