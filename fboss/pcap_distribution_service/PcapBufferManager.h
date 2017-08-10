#pragma once
#include "fboss/pcap_distribution_service/PcapCircularBuffer.h"

#include "fboss/pcap_distribution_service/if/gen-cpp2/pcap_pubsub_types.h"

#include <map>

namespace facebook { namespace fboss {

class PcapBufferManager {
 public:
  PcapBufferManager();
  void addPkt(PcapPkt&& pkt, uint16_t ethertype);

 private:
  std::map<uint16_t, PcapCircularBuffer> buffers_;
  uint16_t UNKNOWN = 0xFFFF;
};
}}
