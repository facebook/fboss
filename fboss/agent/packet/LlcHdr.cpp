/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/packet/LlcHdr.h"

namespace facebook { namespace fboss {

using folly::io::Cursor;

LlcHdr::LlcHdr(Cursor& cursor) {
  try {
    dsap = cursor.read<uint8_t>();
    ssap = cursor.read<uint8_t>();
    if (ssap == LLC_SAP_GLOBAL) {
      throw HdrParseError("LLC: SSAP cannot be the global address (0xFF)");
    }
    control = cursor.read<uint8_t>();
    if (control != LLC_CONTROL_UI) {
      throw HdrParseError("LLC: only Type 1 'UI' PDUs are supported");
    }
  } catch (const std::out_of_range& e) {
    throw HdrParseError("LLC header too small");
  }
}

}} // facebook::fboss
