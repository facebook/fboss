/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"

#include "fboss/agent/FbossError.h"

namespace facebook { namespace fboss {

std::pair<std::shared_ptr<SwitchState>, std::string> applyThriftConfigDefault(
    std::shared_ptr<SwitchState>,
    const Platform*,
    const cfg::SwitchConfig* /*prevConfig*/) {
  throw FbossError("Must specify a configuration file with --config");
}

}} // facebook::fboss
