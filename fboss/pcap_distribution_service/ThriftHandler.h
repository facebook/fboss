#pragma once

#include <thrift/lib/cpp/server/TServer.h>
#include <memory>

#include "fboss/pcap_distribution_service/if/gen-cpp2/PcapPushSubscriber.h"

namespace facebook { namespace fboss {

class PcapDistributor;
class PcapBufferManager;

/*
 * This class handles users connecting to the service,
 * and SwSwitch connecting to the service to send packets.
 */
class ThriftHandler : virtual public PcapPushSubscriberSvIf,
                    public apache::thrift::server::TServerEventHandler {
 public:
  explicit ThriftHandler(
      std::unique_ptr<PcapDistributor> d,
      std::unique_ptr<PcapBufferManager> b)
      : dist_(std::move(d)), buffMgr_(std::move(b)) {}
  /*
   * Called by clients to subscribe to the distribution service
   */
  void subscribe(std::unique_ptr<std::string> hostname, int port) override;
  void unsubscribe(std::unique_ptr<std::string> hostname, int port) override;

  /*
   * Called by SwSwitch when a packet is received
   */
  void receiveRxPacket(std::unique_ptr<RxPacketData> pkt, int16_t ethertype)
      override;
  void receiveTxPacket(std::unique_ptr<TxPacketData> pkt, int16_t ethertype)
      override;
  /*
   * A thrift kill switch for the service
   */
  void kill() override;

  /*
   * Methods to receive a packet dump from the service
   */
  void dumpAllPackets(std::vector<CapturedPacket>& out) override;
  void dumpPacketsByType(
      std::vector<CapturedPacket>& out,
      std::unique_ptr<std::vector<int16_t>> ethertypes) override;

 private:
  std::unique_ptr<PcapDistributor> dist_;
  std::unique_ptr<PcapBufferManager> buffMgr_;
};
}}
