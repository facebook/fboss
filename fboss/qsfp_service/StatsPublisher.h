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

#include "fboss/qsfp_service/TransceiverManager.h"

namespace facebook { namespace fboss {
class StatsPublisher {
 public:
  explicit StatsPublisher(TransceiverManager* transceiverManager)
    : transceiverManager_(transceiverManager) {};
  void init();
  void publishStats();
  static void bumpPciLockHeld();
  static void bumpReadFailure();
  static void bumpWriteFailure();
  static void missingPorts(TransceiverID module);

 private:
  TransceiverManager* transceiverManager_{nullptr};
};
}}
