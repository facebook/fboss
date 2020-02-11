#pragma once

#include <boost/circular_buffer.hpp>

#include "fboss/agent/capture/PcapPkt.h"

#include "folly/Synchronized.h"

#include <memory>
#include <mutex>

namespace facebook { namespace fboss {

class PcapCircularBuffer {
 public:
  explicit PcapCircularBuffer(int n = 100)
      : buf_(boost::circular_buffer<PcapPkt>(n)) {}

  void addPkt(PcapPkt pkt) {
    buf_.wlock()->push_back(std::move(pkt));
  }

  void resize(int n) {
    buf_.wlock()->set_capacity(n);
  }

  int size() {
    return buf_.rlock()->size();
  }

  int capacity() {
    return buf_.rlock()->capacity();
  }

  // release the buffer to a user to dump, and swap in
  // a new buffer
  boost::circular_buffer<PcapPkt> release() {
    return buf_.copy();
  }

  // can add new functions, such as get after timestamp

 private:
  folly::Synchronized<boost::circular_buffer<PcapPkt>> buf_;
};

}}
