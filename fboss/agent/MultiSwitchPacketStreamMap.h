// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/if/gen-cpp2/MultiSwitchCtrl.h"
#include "fboss/agent/types.h"

#include <unordered_map>

namespace facebook::fboss {

class MultiSwitchPacketStreamMap {
 public:
  void addPacketStream(
      SwitchID switchId,
      std::unique_ptr<
          apache::thrift::ServerStreamPublisher<multiswitch::TxPacket>> stream);
  void removePacketStream(SwitchID switchId);
  apache::thrift::ServerStreamPublisher<multiswitch::TxPacket>& getStream(
      SwitchID switchId) const;

 private:
  std::unordered_map<
      SwitchID,
      std::unique_ptr<
          apache::thrift::ServerStreamPublisher<multiswitch::TxPacket>>>
      txPacketStreamMap_;
};

} // namespace facebook::fboss
