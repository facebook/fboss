// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/MultiSwitchPacketStreamMap.h"

#include "fboss/agent/FbossError.h"

using apache::thrift::ServerStream;
using apache::thrift::ServerStreamPublisher;

namespace facebook::fboss {
void MultiSwitchPacketStreamMap::addPacketStream(
    SwitchID switchId,
    std::unique_ptr<
        apache::thrift::ServerStreamPublisher<multiswitch::TxPacket>> stream) {
  if (txPacketStreamMap_.find(switchId) != txPacketStreamMap_.end()) {
    txPacketStreamMap_.erase(switchId);
  }
  txPacketStreamMap_.insert({switchId, std::move(stream)});
}

void MultiSwitchPacketStreamMap::removePacketStream(SwitchID switchId) {
  if (txPacketStreamMap_.find(switchId) != txPacketStreamMap_.end()) {
    txPacketStreamMap_.erase(switchId);
  }
}

ServerStreamPublisher<multiswitch::TxPacket>&
MultiSwitchPacketStreamMap::getStream(SwitchID switchId) const {
  auto it = txPacketStreamMap_.find(switchId);
  if (it == txPacketStreamMap_.end()) {
    throw FbossError("No stream found for switchId ", switchId);
  }
  return *it->second;
}
} // namespace facebook::fboss
