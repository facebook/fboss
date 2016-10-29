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
#include "fboss/qsfp_service/sff/Transceiver.h"
#include <folly/Memory.h>
#include "fboss/agent/FbossError.h"

namespace facebook { namespace fboss {

TransceiverIdx TransceiverMap::transceiverMapping(PortID portID) const {
  std::lock_guard<std::mutex> guard(mutex_);
  auto it = transceiverMap_.find(portID);
  if (it == transceiverMap_.end()) {
    throw FbossError("No transceiver entry found for port ", portID);
  }
  return it->second;
}

Transceiver* TransceiverMap::transceiver(TransceiverID idx) const {
  std::lock_guard<std::mutex> guard(mutex_);
  return transceivers_.at(idx).get();
}

void TransceiverMap::addTransceiverMapping(
    PortID portID, ChannelID channel, TransceiverID module) {
  std::lock_guard<std::mutex> guard(mutex_);
  transceiverMap_.emplace(portID, std::make_pair(channel, module));
}

void TransceiverMap::addTransceiver(
    TransceiverID idx,
    std::unique_ptr<Transceiver> module) {
  CHECK(module) << "Got nullptr to transceiver module.";

  std::lock_guard<std::mutex> guard(mutex_);

  // Try emplacing. If one already exists then throw an error
  auto ret = transceivers_.emplace(idx, std::move(module));
  if (not ret.second) {
    throw FbossError("Duplicate transceivers found for id ", idx);
  }
}

void TransceiverMap::iterateTransceivers(
    std::function<void(TransceiverID, Transceiver*)> callback) const {
  std::lock_guard<std::mutex> guard(mutex_);
  for (auto const& iter : transceivers_) {
    callback(iter.first, iter.second.get());
  }
}

}} // facebook::fboss
