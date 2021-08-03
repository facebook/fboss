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

#include "fboss/cli/fboss2/CmdSubcommands.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

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

  RetType queryClientHelper(const folly::IPAddress& hostIp) {
    RetType result;
    if constexpr (
        ObjectArgTypeId ==
        utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IPV6_LIST) {
      result = impl().queryClient(
          hostIp, CmdSubcommands::getInstance()->getIpv6Addrs());
    } else if constexpr (
        ObjectArgTypeId ==
        utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST) {
      result =
          impl().queryClient(hostIp, CmdSubcommands::getInstance()->getPorts());
    } else {
      result = impl().queryClient(hostIp);
    }

    return result;
  }

  std::tuple<std::string, RetType, std::string> asyncHandler(
      const std::string& host) {
    // Derive IP of the supplied host.
    auto hostIp = utils::getIPFromHost(host);
    XLOG(DBG2) << "host: " << host << " ip: " << hostIp.str();

    std::string errStr;
    RetType result;
    try {
      result = queryClientHelper(hostIp);
    } catch (std::exception const& err) {
      errStr = folly::to<std::string>("Thrift call failed: '", err.what(), "'");
    }

    return std::make_tuple(host, result, errStr);
  }
};

} // namespace facebook::fboss
