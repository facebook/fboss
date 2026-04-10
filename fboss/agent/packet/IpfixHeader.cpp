/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/packet/IpfixHeader.h"

#include "fboss/agent/packet/HdrParseError.h"

using namespace folly::io;

namespace facebook::fboss::psamp {

void IpfixHeader::serialize(RWPrivateCursor* cursor) const {
  cursor->writeBE<uint16_t>(version);
  cursor->writeBE<uint16_t>(length);
  cursor->writeBE<uint32_t>(exportTime);
  cursor->writeBE<uint32_t>(sequenceNumber);
  cursor->writeBE<uint32_t>(observationDomainId);
}

uint32_t IpfixHeader::size() const {
  return 16;
}

IpfixHeader IpfixHeader::deserialize(Cursor& cursor) {
  if (cursor.totalLength() < 16) {
    throw HdrParseError(
        "IPFIX header too small: need 16 bytes, have " +
        std::to_string(cursor.totalLength()));
  }

  IpfixHeader hdr;
  hdr.version = cursor.readBE<uint16_t>();
  if (hdr.version != IPFIX_VERSION) {
    throw HdrParseError(
        "Unsupported IPFIX version: " + std::to_string(hdr.version));
  }
  hdr.length = cursor.readBE<uint16_t>();
  hdr.exportTime = cursor.readBE<uint32_t>();
  hdr.sequenceNumber = cursor.readBE<uint32_t>();
  hdr.observationDomainId = cursor.readBE<uint32_t>();
  return hdr;
}

} // namespace facebook::fboss::psamp
