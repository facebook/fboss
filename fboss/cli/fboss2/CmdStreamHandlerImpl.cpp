/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdStreamHandler.cpp"
#include "fboss/cli/fboss2/commands/stream/fsdb/CmdStreamSubFsdbOperState.h"
#include "fboss/cli/fboss2/commands/stream/fsdb/CmdStreamSubFsdbOperStats.h"

namespace facebook::fboss {

template void CmdStreamHandler<
    CmdStreamSubFsdbOperStats,
    CmdStreamSubFsdbOperStatsTraits>::run(std::ostream& out);

template void CmdStreamHandler<
    CmdStreamSubFsdbOperState,
    CmdStreamSubFsdbOperStateTraits>::run(std::ostream& out);

} // namespace facebook::fboss
