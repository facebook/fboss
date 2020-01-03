/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "IPHeaderV4.h"

#include <folly/io/Cursor.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"

using folly::io::Cursor;

namespace facebook::fboss {

void IPHeaderV4::parse(SwSwitch* sw, PortID port, Cursor* cursor) {
  auto len = cursor->pullAtMost(&hdr_[0], sizeof(hdr_));
  if (len != sizeof(hdr_)) {
    sw->portStats(port)->ipv4TooSmall();
    throw FbossError(
        "Too small packet. Get ",
        cursor->length(),
        " bytes. Minimum ",
        sizeof(hdr_),
        " bytes");
  }
  if (getVersion() != 4) {
    sw->portStats(port)->ipv4WrongVer();
    throw FbossError("Wrong IPv4 version number (", getVersion(), " vs 4)");
  }
  // TODO: other sanity checks (i.e. packet length, checksum...)
}

} // namespace facebook::fboss
