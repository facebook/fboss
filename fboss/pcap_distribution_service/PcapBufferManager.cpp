#include "fboss/pcap_distribution_service/PcapBufferManager.h"

#include "fboss/pcap_distribution_service/if/gen-cpp2/pcap_pubsub_types.h"

namespace facebook { namespace fboss {

uint16_t PcapBufferManager::UNKNOWN = 0xFFFF;

PcapBufferManager::PcapBufferManager() {
  for(auto e : PcapBufferManager::getEthertypes()){
    buffers_[e] = PcapCircularBuffer();
  }
}

void PcapBufferManager::addPkt(PcapPkt&& pkt, uint16_t ethertype) {
  auto it = buffers_.find(ethertype);
  if (it == buffers_.end()) {
    buffers_[UNKNOWN].addPkt(std::move(pkt));
  } else {
    it->second.addPkt(std::move(pkt));
  }
}

/*
 * This function will append to the back of the
 * vector reference that is passed in as an argument
 */
void PcapBufferManager::dumpPackets(
    std::vector<CapturedPacket>& out,
    uint16_t ethertype) {
  auto buf = buffers_[ethertype].release();
  for (int i = 0; i < buf.size(); i++) {
    CapturedPacket p;
    p.rx = buf[i].isRx();

    PacketData data;
    folly::IOBuf packetData;

    if (p.rx) {
      RxPacketData r;
      r.srcPort = buf[i].port();
      r.srcVlan = buf[i].vlan();
      buf[i].buf()->cloneInto(packetData);
      r.packetData = packetData.moveToFbString();
      r.reasons = buf[i].getReasons();
      data.set_rxpkt(r);
    } else {
      TxPacketData t;
      buf[i].buf()->cloneInto(packetData);
      t.packetData = packetData.moveToFbString();
      data.set_txpkt(t);
    }
    p.pkt = data;
    out.emplace_back(std::move(p));
  }
}
}}
