/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestNeighborUtils.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss::utility {

bool nbrProgrammedToCpu(const HwSwitch* hwSwitch, const folly::IPAddress& ip) {
  throw facebook::fboss::FbossError("Not imiplemented");
}

std::optional<uint32_t> getNbrClassId(
    const HwSwitch* hwSwitch,
    const folly::IPAddress& ip) {
  throw facebook::fboss::FbossError("Not imiplemented");
}
} // namespace facebook::fboss::utility
