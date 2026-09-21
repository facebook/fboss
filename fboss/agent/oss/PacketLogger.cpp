// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/PacketLogger.h"

#include <memory>

namespace facebook::fboss {
void PacketLogger::log(
    std::string /*pktType*/,
    std::string /*pktDir*/,
    std::optional<VlanID> /*vlanID*/,
    std::string /*srcMac*/,
    std::string /*senderIP*/,
    std::string /*targetIP*/) {
  return;
}
PacketLogger::PacketLogger(SwSwitch* /*sw*/) {}
} // namespace facebook::fboss
