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
  (*txPacketStreamMap_.wlock())[switchId] = std::move(stream);
}

void MultiSwitchPacketStreamMap::removePacketStream(SwitchID switchId) {
  (*txPacketStreamMap_.wlock()).erase(switchId);
}

ServerStreamPublisher<multiswitch::TxPacket>&
MultiSwitchPacketStreamMap::getStream(SwitchID switchId) const {
  auto& txPacketStreamMap = *txPacketStreamMap_.wlock();
  auto it = txPacketStreamMap.find(switchId);
  if (it == txPacketStreamMap.end()) {
    throw FbossError("No stream found for switchId ", switchId);
  }
  return *it->second;
}
} // namespace facebook::fboss
