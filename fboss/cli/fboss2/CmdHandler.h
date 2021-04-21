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

#include "fboss/cli/fboss2/CLI11/CLI.hpp"

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

#include <memory>

namespace facebook::fboss {

template <typename CmdTypeT, typename CmdTypeTraits>
class CmdHandler {
 public:
  using ClientType = typename CmdTypeTraits::ClientType;
  using RetType = typename CmdTypeTraits::RetType;

  void run();

 private:
  CmdTypeT& impl() {
    return static_cast<CmdTypeT&>(*this);
  }
  const CmdTypeT& impl() const {
    return static_cast<const CmdTypeT&>(*this);
  }
};

} // namespace facebook::fboss
