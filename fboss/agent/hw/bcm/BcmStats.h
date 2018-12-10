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

#include "common/stats/ThreadCachedServiceData.h"
#include <folly/ThreadLocal.h>

namespace facebook { namespace fboss {

class BcmStats {
 public:
  BcmStats();

  /**
   * Get the BcmStats for the current thread.
   *
   * This object should only be used from the current thread.  It should never
   * be stored and used in other threads.
   */
  static BcmStats* get() {
    BcmStats* s = stats_.get();
    if (s) {
      return s;
    }
    return createThreadStats();
  }

  void txPktAlloc() {
    txPktAlloc_.addValue(1);
  }
  void txPktFree() {
    txPktFree_.addValue(1);
  }
  void txSent() {
    txSent_.addValue(1);
  }
  void txSentDone(uint64_t q) {
    txSentDone_.addValue(1);
    txQueued_.addValue(q);
  }
  void txError() {
    txErrors_.addValue(1);
  }
  void txPktAllocErrors() {
    txErrors_.addValue(1);
    txPktAllocErrors_.addValue(1);
  }

  void corrParityError() {
    parityErrors_.addValue(1);
    corrParityErrors_.addValue(1);
  }

  void uncorrParityError() {
    parityErrors_.addValue(1);
    uncorrParityErrors_.addValue(1);
  }

  void asicError() {
    asicErrors_.addValue(1);
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmStats(BcmStats const &) = delete;
  BcmStats& operator=(BcmStats const &) = delete;

  typedef
  stats::ThreadCachedServiceData::ThreadLocalStatsMap ThreadLocalStatsMap;
  typedef stats::ThreadCachedServiceData::TLTimeseries TLTimeseries;
  typedef stats::ThreadCachedServiceData::TLHistogram TLHistogram;
  typedef stats::ThreadCachedServiceData::TLCounter TLCounter;

  explicit BcmStats(ThreadLocalStatsMap *map);

  static BcmStats* createThreadStats();


  // Total number of Tx packet allocated right now
  TLTimeseries txPktAlloc_;
  TLTimeseries txPktFree_;
  TLTimeseries txSent_;
  TLTimeseries txSentDone_;
  // Errors in sending packets
  TLTimeseries txErrors_;
  TLTimeseries txPktAllocErrors_;

  // Time spent for each Tx packet queued in HW
  TLHistogram txQueued_;

  // parity errors
  TLTimeseries parityErrors_;
  TLTimeseries corrParityErrors_;
  TLTimeseries uncorrParityErrors_;

  // Other ASIC errors
  TLTimeseries asicErrors_;

  static folly::ThreadLocalPtr<BcmStats> stats_;
};

}}
