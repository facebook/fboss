// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/PacketObserver.h"
#include "fboss/agent/packet/PktFactory.h"

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <condition_variable>
#include <optional>

namespace facebook::fboss {

class RxPacket;

class HwAgentTestPacketSnooper : public PacketObserverIf {
 public:
  explicit HwAgentTestPacketSnooper(
      PacketObservers* observer,
      std::optional<PortID> port = std::nullopt);
  virtual ~HwAgentTestPacketSnooper() override;
  void packetReceived(const RxPacket* pkt) noexcept override;
  // Wait until timeout (seconds), If timeout = 0, wait forever.
  std::optional<std::unique_ptr<folly::IOBuf>> waitForPacket(
      uint32_t timeout_s);

 private:
  PacketObservers* observers_;
  std::optional<PortID> ingressPort_;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::unique_ptr<folly::IOBuf> data_;
};

} // namespace facebook::fboss
