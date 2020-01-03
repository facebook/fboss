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

#include <folly/IPAddressV4.h>
#include <folly/lang/Bits.h>
#include "fboss/agent/Utils.h"
#include "fboss/agent/types.h"

namespace folly {
namespace io {
class Cursor;
}
} // namespace folly

namespace facebook::fboss {

class SwSwitch;

class IPHeaderV4 {
 public:
  IPHeaderV4() {
    memset(&hdr_[0], 0, sizeof(hdr_));
  }
  void parse(SwSwitch* sw, PortID port, folly::io::Cursor* cursor);

  uint8_t getVersion() const {
    return hdr_[0] >> 4;
  }
  uint8_t getIHL() const {
    return hdr_[0] & 0x0F;
  }
  uint8_t getDSCP() const {
    return hdr_[1] >> 2;
  }
  uint8_t getECN() const {
    return hdr_[1] & 0x3;
  }
  uint16_t getLength() const {
    return read<uint16_t>(2);
  }
  uint16_t getID() const {
    return read<uint16_t>(4);
  }
  uint8_t getFlags() const {
    return hdr_[5] >> 5;
  }
  uint16_t getFragOffset() const {
    return read<uint16_t>(6) & 0x1FFF;
  }
  uint8_t getTTL() const {
    return hdr_[8];
  }
  uint8_t getProto() const {
    return hdr_[9];
  }
  uint16_t getChecksum() const {
    return read<uint16_t>(10);
  }
  folly::IPAddressV4 getSrcIP() const {
    return folly::IPAddressV4::fromLongHBO(read<uint32_t>(12));
  }
  folly::IPAddressV4 getDstIP() const {
    return folly::IPAddressV4::fromLongHBO(read<uint32_t>(16));
  }

 private:
  template <typename T>
  T read(uint32_t pos) const {
    return readBuffer<T>(hdr_, pos, sizeof(hdr_));
  }
  uint8_t hdr_[20];
  // TODO. Options
};

} // namespace facebook::fboss
