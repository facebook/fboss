/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/TransceiverMap.h"

#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Transceiver.h"

namespace facebook::fboss {

TransceiverMap::TransceiverMap() {}

TransceiverMap::~TransceiverMap() {}

MultiSwitchTransceiverMap* MultiSwitchTransceiverMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::transceiverMaps>(state);
}

template class ThriftMapNode<TransceiverMap, TransceiverMapTraits>;

} // namespace facebook::fboss
