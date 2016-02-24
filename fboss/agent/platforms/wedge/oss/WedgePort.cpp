/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/WedgePort.h"

namespace facebook { namespace fboss {

void WedgePort::linkStatusChanged(bool up, bool adminUp) {
  // TODO: The LED handling code isn't open source yet, but should be soon
  // once we get approval for the required OpenNSL APIs.
}

void WedgePort::remedy() {}

void WedgePort::prepareForWarmboot() {}

}} // facebook::fboss
