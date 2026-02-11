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

#include "fboss/cli/fboss2/CmdHandler.h"

namespace facebook::fboss {

struct CmdConfigL2Traits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = std::string;
};

class CmdConfigL2 : public CmdHandler<CmdConfigL2, CmdConfigL2Traits> {
 public:
  using ObjectArgType = CmdConfigL2Traits::ObjectArgType;
  using RetType = CmdConfigL2Traits::RetType;

  RetType queryClient(const HostInfo& /* hostInfo */) {
    throw std::runtime_error(
        "Incomplete command, please use 'learning-mode' subcommand");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
