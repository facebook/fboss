/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/hwobject/uncached/CmdShowHwObjectUncached.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdShowHwObjectUncached::RetType CmdShowHwObjectUncached::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedHwObjectTypes) {
  return queryHwObjects(hostInfo, queriedHwObjectTypes, false /*cached*/);
}

void CmdShowHwObjectUncached::printOutput(
    const RetType& hwObjectInfo,
    std::ostream& out) {
  out << hwObjectInfo << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdShowHwObjectUncached, CmdShowHwObjectUncachedTraits>::run();
} // namespace facebook::fboss
