/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/PortFlowletConfig.h"
#include "fboss/agent/state/NodeBase-defs.h"

namespace facebook::fboss {

template struct ThriftStructNode<PortFlowletCfg, state::PortFlowletFields>;

} // namespace facebook::fboss
