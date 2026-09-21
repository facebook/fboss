/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/PortFlowletConfigMap.h"

#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/PortFlowletConfig.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

MultiSwitchPortFlowletCfgMap* MultiSwitchPortFlowletCfgMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::portFlowletCfgMaps>(state);
}

template struct ThriftMapNode<PortFlowletCfgMap, PortFlowletCfgMapTraits>;

} // namespace facebook::fboss
