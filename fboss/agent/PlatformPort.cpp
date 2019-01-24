/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/PlatformPort.h"

namespace facebook { namespace fboss {

std::ostream& operator<<(std::ostream& os, PlatformPort::ExternalState lfs) {
  switch (lfs) {
    case PlatformPort::ExternalState::NONE:
      os << "None";
      break;
    case PlatformPort::ExternalState::CABLING_ERROR:
      os << "Cabling Error";
      break;
  }
  return os;
}

}} // facebook::fboss
