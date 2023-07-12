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

inline folly::StringPiece constexpr kInPfc() {
  return "in_pfc_frames";
}

inline folly::StringPiece constexpr kInPfcXon() {
  return "in_pfc_xon_frames";
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

inline folly::StringPiece constexpr kOutPfc() {
  return "out_pfc_frames";
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

inline folly::StringPiece constexpr kCreditWatchdogDeletedPackets() {
  return "credit_watchdog_deleted_packets";
}

inline folly::StringPiece constexpr kFecCorrectable() {
  return "fec_correctable_errors";
}

inline folly::StringPiece constexpr kFecUncorrectable() {
  return "fec_uncorrectable_errors";
}

inline folly::StringPiece constexpr kInLabelMissDiscards() {
  return "in_label_miss_discards";
}

inline folly::StringPiece constexpr kWredDroppedPackets() {
  return "wred_dropped_packets";
}

inline folly::StringPiece constexpr kObmLossyHighPriDroppedPkts() {
  return "obm_lossy_high_pri_dropped_pkts";
}

inline folly::StringPiece constexpr kObmLossyHighPriDroppedBytes() {
  return "obm_lossy_high_pri_dropped_bytes";
}

inline folly::StringPiece constexpr kObmLossyLowPriDroppedPkts() {
  return "obm_lossy_low_pri_dropped_pkts";
}

inline folly::StringPiece constexpr kObmLossyLowPriDroppedBytes() {
  return "obm_lossy_low_pri_dropped_bytes";
}

inline folly::StringPiece constexpr kObmHighWatermark() {
  return "obm_high_watermark";
}

inline folly::StringPiece constexpr kErrorsPerCodeword() {
  return "errors_per_codeword";
}

/**
 * Maximum FEC errors we can ever see under any config
 */
constexpr int kMaxFecErrors = 16;
/*
 * Macsec constants
 */

inline folly::StringPiece constexpr kInPreMacsecDropPkts() {
  return "in_premacsec_drop_pkts";
}

inline folly::StringPiece constexpr kInMacsecControlPkts() {
  return "in_macsec_control_pkts";
}

inline folly::StringPiece constexpr kInMacsecDataPkts() {
  return "in_macsec_data_pkts";
}

inline folly::StringPiece constexpr kInMacsecDecryptedBytes() {
  return "in_macsec_decrypted_bytes";
}

inline folly::StringPiece constexpr kInMacsecBadOrNoTagDroppedPkts() {
  return "in_macsec_no_or_bad_tag_dropped_pkts";
}

inline folly::StringPiece constexpr kInMacsecNoSciDroppedPkts() {
  return "in_macsec_no_sci_dropped_pkts";
}

inline folly::StringPiece constexpr kInMacsecUnknownSciPkts() {
  return "in_macsec_unknonwn_sci_pkts";
}

inline folly::StringPiece constexpr kInMacsecOverrunDroppedPkts() {
  return "in_macsec_overrun_dropped_pkts";
}

inline folly::StringPiece constexpr kInMacsecDelayedPkts() {
  return "in_macsec_delayed_pkts";
}

inline folly::StringPiece constexpr kInMacsecLateDroppedPkts() {
  return "in_macsec_late_dropped_pkts";
}

inline folly::StringPiece constexpr kInMacsecNotValidDroppedPkts() {
  return "in_macsec_not_valid_dropped_pkts";
}

inline folly::StringPiece constexpr kInMacsecInvalidPkts() {
  return "in_macsec_invalid_dropped_pkts";
}

inline folly::StringPiece constexpr kInMacsecNoSADroppedPkts() {
  return "in_macsec_no_sa_dropped_pkts";
}

inline folly::StringPiece constexpr kInMacsecUnusedSAPkts() {
  return "in_macsec_unused_sa_pkts";
}

inline folly::StringPiece constexpr kInMacsecUntaggedPkts() {
  return "in_macsec_untagged_pkts";
}

inline folly::StringPiece constexpr kInMacsecCurrentXpn() {
  return "in_macsec_sa_current_xpn";
}

inline folly::StringPiece constexpr kOutPreMacsecDropPkts() {
  return "out_premacsec_drop_pkts";
}

inline folly::StringPiece constexpr kOutMacsecControlPkts() {
  return "out_macsec_control_pkts";
}

inline folly::StringPiece constexpr kOutMacsecDataPkts() {
  return "out_macsec_data_pkts";
}

inline folly::StringPiece constexpr kOutMacsecEncryptedBytes() {
  return "out_macsec_encrypted_bytes";
}
inline folly::StringPiece constexpr kOutMacsecUntaggedPkts() {
  return "out_macsec_untagged_pkts";
}

inline folly::StringPiece constexpr kOutMacsecTooLongDroppedPkts() {
  return "out_macsec_too_long_dropped_pkts";
}

inline folly::StringPiece constexpr kOutMacsecCurrentXpn() {
  return "out_macsec_sa_current_xpn";
}

inline folly::StringPiece constexpr kInCongestionDiscards() {
  return "in_congestion_discards";
}
} // namespace facebook::fboss
