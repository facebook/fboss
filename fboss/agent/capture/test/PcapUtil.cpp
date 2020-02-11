/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/capture/test/PcapUtil.h"

#include "fboss/agent/FbossError.h"

#include <folly/ScopeGuard.h>

using std::vector;

namespace facebook::fboss {

vector<PcapPktInfo> readPcapFile(const char* path) {
  char errbuf[PCAP_ERRBUF_SIZE]{0};
  pcap_t* pcap = pcap_open_offline(path, errbuf);
  if (pcap == nullptr) {
    throw FbossError("failed to open pcap file: ", errbuf);
  }
  SCOPE_EXIT {
    pcap_close(pcap);
  };

  vector<PcapPktInfo> pkts;
  while (true) {
    // Return code values from pcap_next_ex:
    // -2: EOF
    // -1: error
    // 0: no packets arrived in timeout (shouldn't happen for offline files)
    // >0: number of packets processed
    struct pcap_pkthdr* hdr;
    const uint8_t* data;
    int rc = pcap_next_ex(pcap, &hdr, &data);
    if (rc == -2) {
      // EOF
      break;
    } else if (rc <= 0) {
      throw FbossError("error reading from pcap file");
    }

    pkts.emplace_back(hdr, data);
  }
  return pkts;
}

} // namespace facebook::fboss
