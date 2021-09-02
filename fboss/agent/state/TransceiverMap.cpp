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

void TransceiverMap::addTransceiver(const std::shared_ptr<Transceiver>& tcvr) {
  addNode(tcvr);
}

void TransceiverMap::updateTransceiver(
    const std::shared_ptr<Transceiver>& tcvr) {
  updateNode(tcvr);
}

TransceiverMap* TransceiverMap::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newTransceivers = clone();
  auto* ptr = newTransceivers.get();
  (*state)->resetTransceivers(std::move(newTransceivers));
  return ptr;
}

FBOSS_INSTANTIATE_NODE_MAP(TransceiverMap, TransceiverMapTraits);

} // namespace facebook::fboss
