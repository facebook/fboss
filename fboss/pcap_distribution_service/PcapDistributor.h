#pragma once

#include <memory>
#include <map>

#include <thrift/lib/cpp2/async/RequestChannel.h>

#include "fboss/pcap_distribution_service/if/gen-cpp2/PcapPushSubscriber.h"
#include "fboss/pcap_distribution_service/if/gen-cpp2/PcapSubscriber.h"
#include "fboss/pcap_distribution_service/if/gen-cpp2/pcap_pubsub_constants.h"

#include "folly/Synchronized.h"

namespace facebook { namespace fboss {

class ChannelCloserCB;

class PcapDistributor {
  /*
   *
   * This class handles subscriptions from clients, and distributes
   * packets received from the switch to all clients.
   *
   */
 public:
  explicit PcapDistributor(std::shared_ptr<folly::EventBase> b) : evb_(b) {}
  void subscribe(std::unique_ptr<std::string> hostname, int port);
  void unsubscribe(const std::string& hostname, int port);
  void distributeRxPacket(RxPacketData* packetData);
  void distributeTxPacket(TxPacketData* packetData);

 private:
  /*
   * This class handles the callback to unsubscribe a client
   * whenever they disconnect from the distribution service
   */
  class ChannelCloserCB : public apache::thrift::CloseCallback {
   public:
    ChannelCloserCB(PcapDistributor* p, std::pair<std::string, int> key)
        : p_(p), key_(key) {}
    void channelClosed() override {
      CHECK(p_);
      p_->unsubscribe(key_.first, key_.second);
    }

   private:
    PcapDistributor* p_ = nullptr;
    // The pair is the key for each client. When the channel
    // closes, we unsubscribe the client based on this pair.
    const std::pair<std::string, int> key_;
  };

  // Map of hostname and port to client object
  folly::Synchronized<std::map<
      std::pair<std::string, int>,
      std::unique_ptr<PcapSubscriberAsyncClient>>>
      subs_;
  folly::Synchronized<std::map<std::pair<std::string, int>, ChannelCloserCB>>
      callbacks_;
  std::shared_ptr<folly::EventBase> evb_;
};
}}
