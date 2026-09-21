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

#include <folly/File.h>
#include <folly/Range.h>
#include <vector>

namespace facebook::fboss {

class PcapPkt;

/*
 * PcapFile supports writing packets to a file in pcap format.
 *
 * PcapFile uses blocking I/O.  If you are recording packets from a
 * non-blocking thread, you should use PcapWriter instead of using PcapFile
 * directly.
 */
class PcapFile {
 public:
  PcapFile();
  explicit PcapFile(folly::StringPiece path, bool overwriteExisting = false);
  ~PcapFile();

  void close();

  void writeGlobalHeader();
  void writePackets(const std::vector<PcapPkt>& pkt);

  // Move constructor and assignment operator
  PcapFile(PcapFile&&) = default;
  PcapFile& operator=(PcapFile&&) = default;

 private:
  struct PktHeader {
    explicit PktHeader(const PcapPkt& pkt);

    uint32_t timeSec{0};
    uint32_t timeUsec{0};
    uint32_t includedLen{0};
    uint32_t origLen{0};
  };

  // Forbidden copy constructor and assignment operator
  PcapFile(PcapFile const&) = delete;
  PcapFile& operator=(PcapFile const&) = delete;

  static int openFlags(bool overwriteExisting);

  folly::File file_;
};

} // namespace facebook::fboss
