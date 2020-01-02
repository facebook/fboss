/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/capture/PcapFile.h"

#include "fboss/agent/capture/PcapPkt.h"

#include <folly/Exception.h>
#include <folly/FileUtil.h>

#include <chrono>

using folly::IOBuf;
using folly::writeFull;
using folly::writevFull;
using std::chrono::microseconds;
using std::chrono::seconds;

namespace facebook::fboss {

PcapFile::PktHeader::PktHeader(const PcapPkt& pkt) {
  auto ts = pkt.timestamp().time_since_epoch();
  seconds tsSec = std::chrono::duration_cast<seconds>(ts);
  microseconds tsUsec = std::chrono::duration_cast<microseconds>(ts);
  auto len = pkt.buf()->computeChainDataLength();

  timeSec = tsSec.count();
  timeUsec = (tsUsec - tsSec).count();
  includedLen = len;
  origLen = len;
}

PcapFile::PcapFile() {}

PcapFile::PcapFile(folly::StringPiece path, bool overwriteExisting)
    : file_(path.str().c_str(), openFlags(overwriteExisting), 0644) {}

PcapFile::~PcapFile() {}

void PcapFile::close() {
  file_.close();
}

void PcapFile::writeGlobalHeader() {
  struct GlobalHeader {
    uint32_t magic;
    uint16_t versionMajor;
    uint16_t versionMinor;
    uint32_t tzOffset;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t linkType;
  } hdr;
  hdr.magic = 0xa1b2c3d4;
  hdr.versionMajor = 2;
  hdr.versionMinor = 4;
  hdr.tzOffset = 0;
  hdr.sigfigs = 0;
  hdr.snaplen = 0xffff;
  // Link type 1 is ethernet.  Other possible types we might want to use
  // include 113 for linux "cooked" capture format.
  hdr.linkType = 1;

  int ret = writeFull(file_.fd(), &hdr, sizeof(hdr));
  folly::checkUnixError(ret, "error writing pcap global header");
}

void PcapFile::writePackets(const std::vector<PcapPkt>& pkts) {
  folly::fbvector<PktHeader> hdrs;
  hdrs.reserve(pkts.size());
  folly::fbvector<struct iovec> iov;
  // Reserve enough space, assuming each packet is in a single IOBuf.
  // If some packets are split across IOBuf chains then we will end up
  // allocating more space as needed in the loop below.
  iov.reserve(pkts.size() * 2);

  // Build iovecs for all of the packet headers and data
  for (const auto& pkt : pkts) {
    hdrs.emplace_back(pkt);
    PktHeader* curHdr = &hdrs.back();
    iov.push_back({(void*)curHdr, sizeof(PktHeader)});
    pkt.buf()->appendToIov(&iov);
  }

  int ret = writevFull(file_.fd(), iov.data(), iov.size());
  folly::checkUnixError(ret, "error writing pcap data");
}

int PcapFile::openFlags(bool overwriteExisting) {
  int flags = O_CREAT | O_WRONLY;
  if (!overwriteExisting) {
    flags |= O_EXCL;
  }
  return flags;
}

} // namespace facebook::fboss
