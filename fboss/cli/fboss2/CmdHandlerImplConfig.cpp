/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/commands/config/CmdConfigReload.h"

namespace facebook::fboss {

template void CmdHandler<CmdConfigReload, CmdConfigReloadTraits>::run();

} // namespace facebook::fboss
