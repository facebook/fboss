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
    type_ = static_cast<ICMPv6NDPOptionType>(cursor.read<uint8_t>());
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
    ICMPv6NDPOptionType hdr_type = hdr.type();
    XLOG(DBG4) << "NDP Option: " << static_cast<int>(hdr_type);
    switch (hdr_type) {
      case ICMPv6NDPOptionType::ICMPV6_NDP_OPTION_MTU:
        options.mtu.emplace(getMtu(hdr, cursor));
        break;
      case ICMPv6NDPOptionType::ICMPV6_NDP_OPTION_SOURCE_LINK_LAYER_ADDRESS:
        options.sourceLinkLayerAddress.emplace(
            getSourceLinkLayerAddress(hdr, cursor));
        break;
      case ICMPv6NDPOptionType::ICMPV6_NDP_OPTION_REDIRECTED_HEADER:
      case ICMPv6NDPOptionType::ICMPV6_NDP_OPTION_PREFIX_INFORMATION:
      case ICMPv6NDPOptionType::ICMPV6_NDP_OPTION_TARGET_LINK_LAYER_ADDRESS:
        XLOG(WARNING) << "Ignoring NDP Option: "
                      << static_cast<int>(hdr.type());
        skipOption(hdr, cursor);
        break;
      default:
        XLOG(WARNING) << "Ignoring unknown NDP Option: "
                      << static_cast<int>(hdr.type());
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
