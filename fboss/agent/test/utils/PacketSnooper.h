// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/PacketObserver.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/types.h"

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <condition_variable>
#include <optional>

namespace facebook::fboss {
class RxPacket;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

class PacketSnooper : public PacketObserverIf {
 public:
  explicit PacketSnooper(
      std::optional<PortID> port = std::nullopt,
      std::optional<utility::EthFrame> expectedFrame = std::nullopt)
      : port_(port), expectedFrame_(std::move(expectedFrame)) {}

  void packetReceived(const RxPacket* pkt) noexcept override;

  virtual ~PacketSnooper() override = default;
  // Wait until timeout (seconds), If timeout = 0, wait forever.
  std::optional<utility::EthFrame> waitForPacket(uint32_t timeout_s = 0);

 private:
  std::optional<PortID> port_;
  std::optional<utility::EthFrame> expectedFrame_;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::unique_ptr<utility::EthFrame> receivedFrame_;
};

} // namespace facebook::fboss::utility
