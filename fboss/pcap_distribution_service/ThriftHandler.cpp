#include "fboss/pcap_distribution_service/ThriftHandler.h"

#include "fboss/pcap_distribution_service/PcapBufferManager.h"
#include "fboss/pcap_distribution_service/PcapDistributor.h"

#include "fboss/agent/capture/PcapPkt.h"

#include <memory>

using namespace std;

namespace facebook { namespace fboss {

void ThriftHandler::subscribe(unique_ptr<string> hostname, int port) {
  dist_->subscribe(move(hostname), port);
}

void ThriftHandler::unsubscribe(unique_ptr<string> hostname, int port) {
  dist_->unsubscribe(*hostname, port);
}

void ThriftHandler::receiveRxPacket(
    unique_ptr<RxPacketData> pkt,
    int16_t ethertype) {
  dist_->distributeRxPacket(pkt.get());
  buffMgr_->addPkt(PcapPkt(pkt.get()), ethertype);
}

void ThriftHandler::receiveTxPacket(
    unique_ptr<TxPacketData> pkt,
    int16_t ethertype) {
  dist_->distributeTxPacket(pkt.get());
  buffMgr_->addPkt(PcapPkt(pkt.get()), ethertype);
}

void ThriftHandler::kill(){
  LOG(INFO) << "KILL SIGNAL FROM AGENT";
  exit(0);
}

void ThriftHandler::dumpAllPackets(vector<CapturedPacket>& out) {
  for(const auto& type : PcapBufferManager::getEthertypes()) {
    buffMgr_->dumpPackets(out, type);
  }
}

void ThriftHandler::dumpPacketsByType(
    vector<CapturedPacket>& out,
    unique_ptr<vector<int16_t>> ethertypes) {
  for(const auto& type : *ethertypes){
    buffMgr_->dumpPackets(out, type);
  }
}
}}
