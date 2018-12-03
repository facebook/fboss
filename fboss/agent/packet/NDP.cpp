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

namespace facebook {
namespace fboss {

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

uint32_t NDPOptions::getMtu(
    const NDPOptionHdr& ndpHdr,
    folly::io::Cursor& cursor) {
  if (ndpHdr.length() != NDPOptionLength::MTU) {
    throw HdrParseError("Bad length for NDP option MTU");
  }
  // This reserved field *must* be ignored by the receiver (RFC4861 4.6.4)
  cursor.skip(sizeof(uint16_t));
  return cursor.readBE<uint32_t>();
}

folly::MacAddress NDPOptions::getSourceLinkLayerAddress(
    const NDPOptionHdr& ndpHdr,
    folly::io::Cursor& cursor) {
  if (ndpHdr.length() != NDPOptionLength::SRC_LL_ADDRESS_IEEE802) {
    throw HdrParseError("Bad length for NDP option SrcLLAdr");
  }
  return PktUtil::readMac(&cursor);
}

void NDPOptions::skipOption(
    const NDPOptionHdr& ndpHdr,
    folly::io::Cursor& cursor) {
  cursor += ndpHdr.payloadLength();
}

NDPOptions NDPOptions::tryGetAll(folly::io::Cursor& cursor) {
  NDPOptions options;
  while (cursor.length()) {
    auto hdr = NDPOptionHdr(cursor);
    XLOG(DBG4) << "NDP Option: " << hdr.type();
    switch (hdr.type()) {
      case NDPOptionType::MTU:
        options.mtu.emplace(getMtu(hdr, cursor));
        break;
      case NDPOptionType::SRC_LL_ADDRESS:
        options.sourceLinkLayerAddress.emplace(
            getSourceLinkLayerAddress(hdr, cursor));
        break;
      case NDPOptionType::TARGET_LL_ADDRESS:
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
  return options;
}

NDPOptions NDPOptions::getAll(folly::io::Cursor& cursor) {
  try {
    return tryGetAll(cursor);
  } catch (const HdrParseError& e) {
    XLOG(WARNING) << e.what();
  }
  return NDPOptions();
}

} // namespace fboss
} // namespace facebook
