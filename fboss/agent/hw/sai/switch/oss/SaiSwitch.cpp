// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace facebook::fboss {
void SaiSwitch::tamEventCallback(
    sai_object_id_t /*tam_event_id*/,
    sai_size_t /*buffer_size*/,
    const void* /*buffer*/,
    uint32_t /*attr_count*/,
    const sai_attribute_t* /*attr_list*/) {
  // noop;
}

void SaiSwitch::switchEventCallback(
    sai_size_t /*buffer_size*/,
    const void* /*buffer*/,
    uint32_t /*event_type*/) {
  // noop;
}

void SaiSwitch::hardResetSwitchEventNotificationCallback(
    sai_size_t /*bufferSize*/,
    const void* /*buffer*/) {
  // noop;
}

void SaiSwitch::initTechSupport() {}

PacketType getReceivedPacketType(int32_t packetType) {
  throw FbossError(
      "Received packet type conversion is unsupported : ", packetType);
}

bool SaiSwitch::sendPacketOutOfPortSyncForPktType(
    std::unique_ptr<TxPacket> /*pkt*/,
    const PortID& /*portID*/,
    PacketType /*packetType*/) {
  throw FbossError("Sending packet over fabric is unsupported for platform!");
}

} // namespace facebook::fboss
