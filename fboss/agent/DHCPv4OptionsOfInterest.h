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

namespace facebook::fboss {

enum DHCPv4OptionsOfInterest {
  PAD = 0,
  DHCP_MESSAGE_TYPE = 53,
  DHCP_MAX_MESSAGE_SIZE = 57,
  DHCP_AGENT_OPTIONS = 82,
  END = 255
};

} // namespace facebook::fboss
