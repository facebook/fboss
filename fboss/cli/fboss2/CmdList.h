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

#include <functional>
#include <string>
#include <tuple>
#include <vector>

#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

using CommandHandlerFn = std::function<void()>;

using CmdVerb = std::string;
using CmdObject = std::string;
using CmdSubCmd = std::string;
using CmdHelpMsg = std::string;

const std::vector<std::tuple<
    std::string,
    std::string,
    utils::ObjectArgTypeId,
    std::string,
    std::string,
    CommandHandlerFn>>&
kListOfCommands();
const std::vector<std::tuple<
    std::string,
    std::string,
    utils::ObjectArgTypeId,
    std::string,
    std::string,
    CommandHandlerFn>>&
kListOfAdditionalCommands();

template <typename T>
void commandHandler() {
  T().run();
}

} // namespace facebook::fboss
