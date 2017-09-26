/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "MockPlatformPort.h"

namespace facebook { namespace fboss {
folly::Future<TransceiverInfo> MockPlatformPort::getTransceiverInfo_(
    folly::EventBase* evb) {
  TransceiverInfo info;
  info.present = transceiverPresent;
  return folly::makeFuture<TransceiverInfo>(std::move(info));
}
}} // facebook::fboss
