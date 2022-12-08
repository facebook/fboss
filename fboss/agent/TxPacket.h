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

#include <folly/MacAddress.h>
#include "fboss/agent/Packet.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

/*
 * TxPacket represents a packet that may be transmitted out via the switch.
 */
class TxPacket : public Packet {
 public:
  /**
   * Write an ethernet header at the specified cursor location.
   *
   * This version writes a header with a VLAN tag.
   * The packet must include enough space for the header (20 bytes for this
   * version which includes a VLAN tag).
   *
   * On return, the cursor will be updated to point just past the end of the
   * header, to the start of the payload data.
   */
  template <typename CursorType>
  static void writeEthHeader(
      CursorType* cursor,
      folly::MacAddress dst,
      folly::MacAddress src,
      VlanID vlan,
      uint16_t protocol);

  /**
   * Write an ethernet header at the specified cursor location.
   *
   * This version writes an untagged header.
   * The packet must include enough space for the header (16 bytes).
   *
   * On return, the cursor will be updated to point just past the end of the
   * header, to the start of the payload data.
   */
  template <typename CursorType>
  static void writeEthHeader(
      CursorType* cursor,
      folly::MacAddress dst,
      folly::MacAddress src,
      uint16_t protocol);

  /**
   * Write an ethernet header at the specified cursor location.
   *
   * This version writes a tagged or untagged header depending on the datatype
   * of input argument vlanID
   */
  template <typename CursorType>
  static void writeEthHeader(
      CursorType* cursor,
      folly::MacAddress dst,
      folly::MacAddress src,
      std::optional<VlanID> vlan,
      uint16_t protocol);

 protected:
  TxPacket() {}

 private:
  // Forbidden copy constructor and assignment operator
  TxPacket(TxPacket const&) = delete;
  TxPacket& operator=(TxPacket const&) = delete;
};

template <typename CursorType>
void TxPacket::writeEthHeader(
    CursorType* cursor,
    folly::MacAddress dst,
    folly::MacAddress src,
    VlanID vlan,
    uint16_t protocol) {
  cursor->push(dst.bytes(), folly::MacAddress::SIZE);
  cursor->push(src.bytes(), folly::MacAddress::SIZE);
  cursor->template writeBE<uint16_t>(0x8100); // 802.1Q
  cursor->template writeBE<uint16_t>(vlan);
  cursor->template writeBE<uint16_t>(protocol);
}

template <typename CursorType>
void TxPacket::writeEthHeader(
    CursorType* cursor,
    folly::MacAddress dst,
    folly::MacAddress src,
    uint16_t protocol) {
  cursor->push(dst.bytes(), folly::MacAddress::SIZE);
  cursor->push(src.bytes(), folly::MacAddress::SIZE);
  cursor->template writeBE<uint16_t>(protocol);
}

template <typename CursorType>
void TxPacket::writeEthHeader(
    CursorType* cursor,
    folly::MacAddress dst,
    folly::MacAddress src,
    std::optional<VlanID> vlan,
    uint16_t protocol) {
  if (vlan.has_value()) {
    writeEthHeader(cursor, dst, src, vlan.value(), protocol);
  } else {
    writeEthHeader(cursor, dst, src, protocol);
  }
}

} // namespace facebook::fboss
