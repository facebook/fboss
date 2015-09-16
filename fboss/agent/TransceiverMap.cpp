/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/TransceiverMap.h"
#include "fboss/agent/Transceiver.h"
#include <folly/Memory.h>
#include "fboss/agent/FbossError.h"

namespace facebook { namespace fboss {

TransceiverMap::TransceiverMap() {
}

TransceiverMap::~TransceiverMap() {
}

TransceiverIdx TransceiverMap::transceiverMapping(PortID portID) const {
  auto it = transceiverMap_.find(portID);
  if (it == transceiverMap_.end()) {
    throw FbossError("No transceiver entry found for port ", portID);
  }
  return it->second;
}

Transceiver* TransceiverMap::transceiver(TransceiverID idx) const {
  return transceivers_.at(idx).get();
}

void TransceiverMap::addTransceiverMapping(PortID portID, ChannelID channel,
                                           TransceiverID module) {
  transceiverMap_.emplace(std::make_pair(portID,
                          std::make_pair(channel, module)));
}

Transceivers::const_iterator TransceiverMap::begin() const {
  return transceivers_.begin();
}

Transceivers::const_iterator TransceiverMap::end() const {
  return transceivers_.end();
}

}} // facebook::fboss
