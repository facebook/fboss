/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/QcmConfig.h"
#include <fboss/agent/gen-cpp2/switch_state_types.h>
#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <fboss/agent/state/Thrifty.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

template struct ThriftStructNode<QcmCfg, state::QcmCfgFields>;

} // namespace facebook::fboss
