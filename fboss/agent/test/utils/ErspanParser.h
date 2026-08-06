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

#include <cstdint>

#include <folly/IPAddress.h>
#include <folly/io/Cursor.h>

namespace facebook::fboss::utility {

// Decoded fields of a Chenab (NVIDIA Spectrum / Minipack3N) ETH-ERSPAN
// mirrored sample, used to cross-validate the mirror-header format the ASIC
// emits against what the sFlow collector parses.
struct ErspanSampleInfo {
  uint32_t ingressLabelPort{};
  uint32_t egressLabelPort{};
  uint32_t originalPacketSize{};
  folly::IPAddress srcAddr;
  folly::IPAddress dstAddr;
  uint8_t ipProtocol{};
  uint16_t l4SrcPort{};
  uint16_t l4DstPort{};
};

// Parse a Chenab ETH-ERSPAN mirror datagram whose cursor is positioned at the
// GRE header (as delivered by the collector's raw GRE socket).
//
// Two build-time implementations exist behind this single declaration: the
// internal build (facebook/ErspanParser.cpp) delegates to the production
// nettools sFlow collector parser, while the open-source build
// (oss/ErspanParser.cpp) uses a self-contained parser. Both return the same
// decoded fields, so the test validates the wire format either way.
ErspanSampleInfo parseChenabErspanSample(folly::io::Cursor& cursor);

} // namespace facebook::fboss::utility
