/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <pcap/pcap.h>
#include <string>
#include <vector>

namespace facebook::fboss {

struct PcapPktInfo {
  PcapPktInfo(const struct pcap_pkthdr* h, const uint8_t* d)
      : hdr(*h), data(reinterpret_cast<const char*>(d), h->caplen) {}

  struct pcap_pkthdr hdr;
  std::string data;
};

std::vector<PcapPktInfo> readPcapFile(const char* path);

} // namespace facebook::fboss
