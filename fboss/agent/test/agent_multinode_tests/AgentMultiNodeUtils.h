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

#include "fboss/agent/if/gen-cpp2/FbossHwCtrl.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"

namespace facebook::fboss::utility {

bool verifySwSwitchRunState(
    const std::string& switchName,
    const SwitchRunState& expectedSwitchRunState);
bool verifyQsfpServiceRunState(
    const std::string& switchName,
    const QsfpServiceRunState& expectedQsfpRunState);
bool verifyFsdbIsUp(const std::string& switchName);

std::vector<NdpEntryThrift> getNdpEntriesOfType(
    const std::string& rdsw,
    const std::set<std::string>& types);

} // namespace facebook::fboss::utility
