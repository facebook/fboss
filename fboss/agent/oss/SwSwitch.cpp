/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SwSwitch.h"
#include <folly/Range.h>
#include <folly/ThreadName.h>

namespace facebook { namespace fboss {

void SwSwitch::initThread(folly::StringPiece name) {
  // We need a null-terminated string to pass to folly::setThreadName().
  // The pthread name can be at most 15 bytes long, so truncate it if necessary
  // when creating the string.
  size_t pthreadLength = std::min(name.size(), (size_t)15);
  char pthreadName[pthreadLength + 1];
  memcpy(pthreadName, name.begin(), pthreadLength);
  pthreadName[pthreadLength] = '\0';
  folly::setThreadName(pthread_self(), pthreadName);
}

void SwSwitch::publishStats() {}

void SwSwitch::publishBootType() {}

}} // facebook::fboss
