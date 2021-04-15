/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <string>
#include <tuple>
#include <vector>

namespace facebook::fboss {

const std::vector<std::tuple<std::string, std::string, std::string>>&
kListOfCommands();
}
