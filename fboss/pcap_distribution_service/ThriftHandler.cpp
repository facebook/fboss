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

void ThriftHandler::receiveRxPacket(unique_ptr<RxPacketData> pkt) {
  dist_->distributeRxPacket(pkt.get());
  // TODO : CHANGE LATER DUMMY VALUE
  buffMgr_->addPkt(PcapPkt(pkt.get()), 0);
}

void ThriftHandler::receiveTxPacket(unique_ptr<TxPacketData> pkt) {
  dist_->distributeTxPacket(pkt.get());
  // TODO : CHANGE LATER DUMMY VALUE
  buffMgr_->addPkt(PcapPkt(pkt.get()), 0);
}

void ThriftHandler::kill(){
  LOG(INFO) << "KILL SIGNAL FROM AGENT";
  exit(0);
}
}}
