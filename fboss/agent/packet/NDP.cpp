/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/NDP.h"

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/packet/HdrParseError.h"
#include "fboss/agent/packet/PktUtil.h"

namespace facebook::fboss {

using folly::IOBuf;
using folly::MacAddress;
using folly::io::Cursor;

NDPOptionHdr::NDPOptionHdr(folly::io::Cursor& cursor) {
  try {
    type_ = cursor.read<uint8_t>();
    length_ = cursor.read<uint8_t>();
  } catch (const std::out_of_range& e) {
    throw HdrParseError("NDP Option header is not present");
  }
  if (length_ == 0) {
    throw HdrParseError("Invalid NDP Option header: length is 0");
  }
  if (!cursor.canAdvance(payloadLength())) {
    throw HdrParseError("NDP Option payload is too small");
  }
}

NDPOptions::NDPOptions(folly::io::Cursor& cursor) {
  tryParse(cursor);
}

void NDPOptions::getMtu(const NDPOptionHdr& ndpHdr, folly::io::Cursor& cursor) {
  if (ndpHdr.length() != NDPOptionLength::MTU) {
    throw HdrParseError("Bad length for NDP option MTU");
  }
  // This reserved field *must* be ignored by the receiver (RFC4861 4.6.4)
  cursor.skip(sizeof(uint16_t));
  mtu = cursor.readBE<uint32_t>();
}

void NDPOptions::getSourceLinkLayerAddress(
    const NDPOptionHdr& ndpHdr,
    folly::io::Cursor& cursor) {
  if (ndpHdr.length() != NDPOptionLength::SRC_LL_ADDRESS_IEEE802) {
    throw HdrParseError("Bad length for NDP option SrcLLAdr");
  }
  sourceLinkLayerAddress = PktUtil::readMac(&cursor);
}

void NDPOptions::getTargetLinkLayerAddress(
    const NDPOptionHdr& ndpHdr,
    folly::io::Cursor& cursor) {
  if (ndpHdr.length() != NDPOptionLength::TARGET_LL_ADDRESS_IEEE802) {
    throw HdrParseError("Bad length for NDP option TargetLLAdr");
  }
  targetLinkLayerAddress = PktUtil::readMac(&cursor);
}

void NDPOptions::skipOption(
    const NDPOptionHdr& ndpHdr,
    folly::io::Cursor& cursor) {
  cursor += ndpHdr.payloadLength();
}

void NDPOptions::tryParse(folly::io::Cursor& cursor) {
  while (cursor.length()) {
    auto hdr = NDPOptionHdr(cursor);
    XLOG(DBG4) << "NDP Option: " << hdr.type();
    switch (hdr.type()) {
      case NDPOptionType::MTU:
        getMtu(hdr, cursor);
        break;
      case NDPOptionType::SRC_LL_ADDRESS:
        getSourceLinkLayerAddress(hdr, cursor);
        break;
      case NDPOptionType::TARGET_LL_ADDRESS:
        getTargetLinkLayerAddress(hdr, cursor);
        break;
      case NDPOptionType::PREFIX_INFO:
      case NDPOptionType::REDIRECTED_HEADER:
        XLOG(WARNING) << "Ignoring NDP Option: " << hdr.type();
        skipOption(hdr, cursor);
        break;
      default:
        XLOG(WARNING) << "Ignoring unknown NDP Option: " << hdr.type();
        skipOption(hdr, cursor);
        break;
    }
  }
}

void NDPOptions::serialize(folly::io::RWPrivateCursor* cursor) const {
  if (mtu) {
    cursor->write<uint8_t>(NDPOptionType::MTU);
    cursor->write<uint8_t>(NDPOptionLength::MTU);
    cursor->writeBE<uint16_t>(0);
    cursor->writeBE<uint32_t>(*mtu);
  }
  if (sourceLinkLayerAddress) {
    cursor->write<uint8_t>(NDPOptionType::SRC_LL_ADDRESS);
    cursor->write<uint8_t>(NDPOptionLength::SRC_LL_ADDRESS_IEEE802);
    cursor->push(sourceLinkLayerAddress->bytes(), MacAddress::SIZE);
  }
  if (targetLinkLayerAddress) {
    cursor->write<uint8_t>(NDPOptionType::TARGET_LL_ADDRESS);
    cursor->write<uint8_t>(NDPOptionLength::TARGET_LL_ADDRESS_IEEE802);
    cursor->push(targetLinkLayerAddress->bytes(), MacAddress::SIZE);
  }
}

size_t NDPOptions::computeTotalLength() const {
  return 8 *
      ((!!mtu * NDPOptionLength::MTU) +
       (!!sourceLinkLayerAddress * NDPOptionLength::SRC_LL_ADDRESS_IEEE802) +
       (!!targetLinkLayerAddress * NDPOptionLength::TARGET_LL_ADDRESS_IEEE802));
}

} // namespace facebook::fboss
