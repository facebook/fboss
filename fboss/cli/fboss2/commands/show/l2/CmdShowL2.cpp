/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "CmdShowL2.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdShowL2::RetType CmdShowL2::queryClient(const HostInfo& /* hostInfo */) {
  return "Please run \"show mac details\" for L2 entries.";
}

void CmdShowL2::printOutput(const RetType& message, std::ostream& out) {
  out << message << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdShowL2, CmdShowL2Traits>::run();
} // namespace facebook::fboss
