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

#include <folly/coro/Task.h>

#include "fboss/cli/fboss2/CmdHandler.h"

namespace facebook::fboss {

template <typename CmdTypeT, typename CmdTypeTraits>
class CmdStreamHandler : public CmdHandler<CmdTypeT, CmdTypeTraits> {
 public:
  using Base = CmdHandler<CmdTypeT, CmdTypeTraits>;
  using Base::getHosts;
  using Base::impl;
  using RetType = typename CmdTypeTraits::RetType;
  using StreamRetType = typename CmdTypeTraits::StreamRetType;

  // add optional parameter `out` for testing purpose.
  // will remove later
  void run(std::ostream& out = std::cout);

 private:
  StreamRetType queryClientHelper(const HostInfo& hostInfo) {
    using ArgTypes = resolve_arg_types<CmdTypeT>;
    auto tupleArgs = CmdArgsLists::getInstance()
                         ->getTypedArgs<
                             typename ArgTypes::unfiltered_type,
                             typename ArgTypes::filtered_type>();
    // Use co_invoke to invoke the lambda to prevent the lambda from being
    // destroyed
    return folly::coro::co_invoke([=, this]() {
      return std::apply(
          [&](auto const&... args) {
            return impl().queryClient(hostInfo, args...);
          },
          tupleArgs);
    });
  }
};

} // namespace facebook::fboss
