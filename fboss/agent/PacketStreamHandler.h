// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/thrift_packet_stream/PacketStreamService.h"

#include <fb303/ThreadCachedServiceData.h>
#include <folly/Synchronized.h>

namespace facebook::fboss {

class SwSwitch;

/**
 * PacketStreamHandler handles bidirectional packet streaming
 * between the FBOSS agent and external agent.
 *
 * Data flows:
 * - FromExternalAgent (Agent -> Handler -> Switch): =
 *   processReceivedPacket() -> sw_->sendNetworkControlPacketAsync()
 * - toExternalAgent (asic -> Handler -> Agent):
 *   handlePacket() called from SwSwitch ethertype dispatch -> send()
 *
 * Packets with ethertype 0x9999 are dispatched to this handler from
 * SwSwitch::handlePacketImpl() (same pattern as MKAServiceManager).
 */
class PacketStreamHandler : public PacketStreamService {
 public:
  // TODO(AIFM_ETHERNET) - update when available
  static constexpr uint16_t ETHERTYPE_AIFM_CTRL = 0x9999;

  PacketStreamHandler(
      SwSwitch* sw,
      const std::string& serviceStream,
      const std::string& statsName);
  ~PacketStreamHandler() override;
  void handlePacket(std::unique_ptr<RxPacket> pkt);

 protected:
  void processReceivedPacket(const std::string& clientId, TPacket&& packet)
      override;
  void clientConnected(const std::string& clientId) override;
  void clientDisconnected(const std::string& clientId) override;
  void addPort(const std::string& clientId, const std::string& l2Port) override;
  void removePort(const std::string& clientId, const std::string& l2Port)
      override;

 private:
  void ensureConfigured(folly::StringPiece function) const;

  SwSwitch* sw_;

  fb303::TimeseriesWrapper fromExternalAgentReceivedCount_;
  fb303::TimeseriesWrapper fromExternalAgentDroppedCount_;
  fb303::TimeseriesWrapper toExternalAgentDroppedCount_;
  fb303::TimeseriesWrapper toExternalAgentSentCount_;
  fb303::TimeseriesWrapper fromExternalAgentSentToHwCount_;
  folly::Synchronized<std::string> connectedClientId_;
  std::atomic<bool> clientConnected_{false};
};

} // namespace facebook::fboss
