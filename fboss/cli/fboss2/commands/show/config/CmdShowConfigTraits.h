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

#include <folly/json/dynamic.h>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

struct CmdShowConfigTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

struct CmdShowConfigVersionTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::map<std::string, std::string>;
};

struct CmdShowConfigDynamicTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = folly::dynamic;
};
} // namespace facebook::fboss
