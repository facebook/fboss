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

#include <folly/io/IOBuf.h>

#include <cstdint>

namespace facebook::fboss {

/*
 * A class for representing network packets.
 *
 * Packet itself is just a base class for RxPacket and TxPacket.  We have
 * separate receive and transmit APIs to support hardware implementations that
 * manage receive and transmit packets separately.
 *
 * The packet data is accessed through an IOBuf.  This allows packets to
 * consist of multiple non-contiguous buffers.  IOBuf's cursor API provides
 * mechanisms for easily reading from and writing to the underlying buffers.
 */
class Packet {
 public:
  virtual ~Packet() {}

  /*
   * Get a folly::IOBuf object pointing to the packet data.
   *
   * Note that if you need to append additional data to the packet, you
   * must use the Packet APIs for this.  You cannot allocate new IOBufs
   * yourself and append them to the chain.  (For instance, HwSwitch
   * implementations may require that packet data be allocated from specific
   * DMA memory ranges.)
   */
  folly::IOBuf* buf() {
    return buf_.get();
  }
  const folly::IOBuf* buf() const {
    return buf_.get();
  }

  /*
   * Used for avoiding copy when sending packet over thrift transport.
   * Packet is not valid after this.
   */
  static std::unique_ptr<folly::IOBuf> extractIOBuf(
      std::unique_ptr<Packet> pkt) {
    return std::move(pkt->buf_);
  }

 protected:
  Packet() {}

 private:
  // Forbidden copy constructor and assignment operator
  Packet(Packet const&) = delete;
  Packet& operator=(Packet const&) = delete;

 protected:
  // An IOBuf pointing to the packet data.
  // We put this directly in the base Packet class, since we want users to be
  // able to access this IOBuf efficiently, without having to make a virtual
  // function call.
  //
  // Subclasses should update the IOBuf to point to their packet data,
  // generally using the TAKE_OWNERSHIP or WRAP_BUFFER IOBuf constructors.
  std::unique_ptr<folly::IOBuf> buf_;
};

} // namespace facebook::fboss
