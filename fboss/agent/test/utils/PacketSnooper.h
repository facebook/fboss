// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/PacketObserver.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/types.h"

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <gtest/gtest.h>
#include <condition_variable>
#include <optional>
#include <queue>

namespace facebook::fboss {
class RxPacket;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

class PacketSnooper : public PacketObserverIf {
 public:
  PacketSnooper(
      std::optional<PortID> port = std::nullopt,
      std::optional<utility::EthFrame> expectedFrame = std::nullopt)
      : port_(port), expectedFrame_(std::move(expectedFrame)) {}

  void packetReceived(const RxPacket* pkt) noexcept override;

  virtual ~PacketSnooper() override {
    EXPECT_TRUE(receivedFrames_.empty());
  }
  // Wait until timeout (seconds), If timeout = 0, wait forever.
  std::optional<utility::EthFrame> waitForPacket(uint32_t timeout_s = 0);

 private:
  std::optional<PortID> port_;
  std::optional<utility::EthFrame> expectedFrame_;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::queue<std::unique_ptr<utility::EthFrame>> receivedFrames_;
};

class SwSwitchPacketSnooper : public PacketSnooper {
 public:
  SwSwitchPacketSnooper(
      SwSwitch* sw,
      const std::string& name,
      std::optional<PortID> port = std::nullopt,
      std::optional<utility::EthFrame> expectedFrame = std::nullopt);

  std::optional<std::unique_ptr<folly::IOBuf>> waitForPacket(
      uint32_t timeout_s);

  virtual ~SwSwitchPacketSnooper() override;

 private:
  SwSwitch* sw_;
  std::string name_;
};

} // namespace facebook::fboss::utility
