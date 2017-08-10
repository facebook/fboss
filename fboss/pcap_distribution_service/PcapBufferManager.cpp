#include "fboss/pcap_distribution_service/PcapBufferManager.h"

#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/IPv4Handler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/LldpManager.h"

namespace facebook { namespace fboss {

PcapBufferManager::PcapBufferManager() {
  buffers_[UNKNOWN] = PcapCircularBuffer();
  buffers_[ArpHandler::ETHERTYPE_ARP] = PcapCircularBuffer();
  buffers_[LldpManager::ETHERTYPE_LLDP] = PcapCircularBuffer();
  buffers_[IPv4Handler::ETHERTYPE_IPV4] = PcapCircularBuffer();
  buffers_[IPv6Handler::ETHERTYPE_IPV6] = PcapCircularBuffer();
}

void PcapBufferManager::addPkt(PcapPkt&& pkt, uint16_t ethertype) {
  auto it = buffers_.find(ethertype);
  if (it == buffers_.end()) {
    buffers_[UNKNOWN].addPkt(std::move(pkt));
  } else {
    it->second.addPkt(std::move(pkt));
  }
}
}}
