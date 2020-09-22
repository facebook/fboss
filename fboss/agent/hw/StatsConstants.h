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

#include <folly/Range.h>

namespace facebook::fboss {

inline folly::StringPiece constexpr kCapacity() {
  return "capacity";
}

inline folly::StringPiece constexpr kInBytes() {
  return "in_bytes";
}

inline folly::StringPiece constexpr kInUnicastPkts() {
  return "in_unicast_pkts";
}

inline folly::StringPiece constexpr kInMulticastPkts() {
  return "in_multicast_pkts";
}

inline folly::StringPiece constexpr kInBroadcastPkts() {
  return "in_broadcast_pkts";
}

inline folly::StringPiece constexpr kInErrors() {
  return "in_errors";
}

inline folly::StringPiece constexpr kInPause() {
  return "in_pause_frames";
}

inline folly::StringPiece constexpr kInIpv4HdrErrors() {
  return "in_ipv4_header_errors";
}

inline folly::StringPiece constexpr kInIpv6HdrErrors() {
  return "in_ipv6_header_errors";
}

inline folly::StringPiece constexpr kInDiscardsRaw() {
  return "in_discards_raw";
}

inline folly::StringPiece constexpr kInDiscards() {
  return "in_discards";
}

inline folly::StringPiece constexpr kInDstNullDiscards() {
  return "in_dst_null_discards";
}

inline folly::StringPiece constexpr kInDroppedPkts() {
  return "in_dropped_pkts";
}

inline folly::StringPiece constexpr kInPkts() {
  return "in_pkts";
}

inline folly::StringPiece constexpr kOutBytes() {
  return "out_bytes";
}

inline folly::StringPiece constexpr kOutUnicastPkts() {
  return "out_unicast_pkts";
}

inline folly::StringPiece constexpr kOutMulticastPkts() {
  return "out_multicast_pkts";
}

inline folly::StringPiece constexpr kOutBroadcastPkts() {
  return "out_broadcast_pkts";
}

inline folly::StringPiece constexpr kOutDiscards() {
  return "out_discards";
}

inline folly::StringPiece constexpr kOutErrors() {
  return "out_errors";
}

inline folly::StringPiece constexpr kOutPause() {
  return "out_pause_frames";
}

inline folly::StringPiece constexpr kOutCongestionDiscards() {
  return "out_congestion_discards";
}

inline folly::StringPiece constexpr kOutCongestionDiscardsBytes() {
  return "out_congestion_discards_bytes";
}

inline folly::StringPiece constexpr kOutEcnCounter() {
  return "out_ecn_counter";
}

inline folly::StringPiece constexpr kOutPkts() {
  return "out_pkts";
}

inline folly::StringPiece constexpr kFecCorrectable() {
  return "fec_correctable_errors";
}

inline folly::StringPiece constexpr kFecUncorrectable() {
  return "fec_uncorrectable_errors";
}

inline folly::StringPiece constexpr kWredDroppedPackets() {
  return "wred_dropped_packets";
}

} // namespace facebook::fboss
