#pragma once
#include "fboss/pcap_distribution_service/PcapCircularBuffer.h"

#include "fboss/pcap_distribution_service/if/gen-cpp2/pcap_pubsub_types.h"

#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/IPv4Handler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/LldpManager.h"

#include <map>
#include <vector>

namespace facebook {
namespace fboss {

class CapturedPacket;

class PcapBufferManager {
 public:
  PcapBufferManager();
  void addPkt(PcapPkt&& pkt, uint16_t ethertype);
  void dumpPackets(std::vector<CapturedPacket>& out, uint16_t ethertype);
  static uint16_t UNKNOWN;
  static const std::vector<uint16_t>& getEthertypes() {
    static const std::vector<uint16_t> ethertypes = {
        PcapBufferManager::UNKNOWN,
        ArpHandler::ETHERTYPE_ARP,
        LldpManager::ETHERTYPE_LLDP,
        IPv4Handler::ETHERTYPE_IPV4,
        IPv6Handler::ETHERTYPE_IPV6};
    return ethertypes;
  }

 private:
  std::map<uint16_t, PcapCircularBuffer> buffers_;
};
}
}
