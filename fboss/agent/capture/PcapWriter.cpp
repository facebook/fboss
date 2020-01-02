/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/capture/PcapWriter.h"

#include "fboss/agent/capture/PcapPkt.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>

using folly::StringPiece;

namespace facebook::fboss {

PcapWriter::PcapWriter(uint32_t maxBufferedPkts) : queue_(maxBufferedPkts) {}

PcapWriter::PcapWriter(
    StringPiece path,
    bool overwriteExisting,
    uint32_t maxBufferedPkts)
    : file_(path, overwriteExisting),
      queue_(maxBufferedPkts),
      thread_(&PcapWriter::threadMain, this) {}

PcapWriter::~PcapWriter() {
  try {
    finish();
  } catch (const std::exception& ex) {
    // Can't throw from a destructor, so just log the error.
    // Ideally the caller should always call finish() explicitly before calling
    // the destructor, so they can handle errors properly.
    XLOG(ERR) << "shutting down PcapWriter with unhandled exception: "
              << folly::exceptionStr(ex);
  }
}

void PcapWriter::start(folly::StringPiece path, bool overwriteExisting) {
  file_ = PcapFile(path, overwriteExisting);
  thread_ = std::thread(&PcapWriter::threadMain, this);
}

void PcapWriter::finish() {
  if (!thread_.joinable()) {
    // already stopped
    return;
  }

  queue_.finish();
  thread_.join();
  if (ex_) {
    std::rethrow_exception(ex_);
  }
}

void PcapWriter::threadMain() {
  try {
    file_.writeGlobalHeader();
    writeLoop();
    file_.close();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "error writing to pcap file: " << folly::exceptionStr(ex);
    ex_ = std::current_exception();
  }
}

void PcapWriter::writeLoop() {
  std::vector<PcapPkt> pkts;
  while (true) {
    pkts.clear();
    if (!queue_.wait(&pkts)) {
      DCHECK(pkts.empty());
      return;
    }

    DCHECK(!pkts.empty());
    file_.writePackets(pkts);
  }
}

} // namespace facebook::fboss
