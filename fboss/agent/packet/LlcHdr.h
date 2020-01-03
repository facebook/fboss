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

/*
 * Logical Link Control
 *
 * This protocol sits directly on top of Ethernet. An Ethernet frame uses LLC
 * if its EtherType is either the actual payload size, i.e. between 0 and 1500
 * octets, or is the special code for a Jumbo frame, i.e. 0x8870.
 *
 * References
 *   https://en.wikipedia.org/wiki/EtherType
 *   https://en.wikipedia.org/wiki/Logical_Link_Control
 *   https://en.wikipedia.org/wiki/IEEE_802.2
 *   http://ckp.made-it.com/ieee8022.html
 *   http://standards.ieee.org/getieee802/download/802.2-1998.pdf
 *   http://www.wildpackets.com/resources/compendium/llc/overview
 */
#include <folly/io/Cursor.h>
#include "fboss/agent/packet/HdrParseError.h"

namespace facebook::fboss {

/*
 * DSAP:
 *   LSB=0: Individual destination address
 *   LSB=1: Group destination address
 * SSAP:
 *   LSB=0: Command message
 *   LSB=1: Response message
 *
 * References:
 *   http://www.wildpackets.com/resources/compendium/reference/sap_numbers
 */
enum class LLC_SAP_ADDR : uint8_t {
  LLC_SAP_NULL = 0x00,
  LLC_SAP_LLC_SUBLAYER_MANAGEMENT = 0x02,
  LLC_SAP_IBM_SNA_PATH_CONTROL = 0x04,
  LLC_SAP_IP = 0x06, // Internet Protocol
  LLC_SAP_PROWAY_NET_MGNT_INIT = 0x0E,
  LLC_SAP_TEXAS_INSTRUMENTS = 0x18,
  LLC_SAP_STP = 0x42, // Spanning Tree Protocol
  LLC_SAP_ISO_8208 = 0x7F,
  LLC_SAP_XEROX_NETWORK_SYSTEMS = 0x80,
  LLC_SAP_NESTAR = 0x86,
  LLC_SAP_PROWAY_ACT_STA_LIST_MNT = 0x8E,
  LLC_SAP_ARP = 0x98, // Address Resolution Protocol
  LLC_SAP_RDE = 0xA6, // Route Determination Entity
  LLC_SAP_SNAP = 0xAA, // Subnetwork Access Protocol
  LLC_SAP_BANYAN_VINES = 0xBC,
  LLC_SAP_NOVELL_NETWARE = 0xE0,
  LLC_SAP_IBM_NETBIOS = 0xF0,
  LLC_SAP_IBM_LAN_MANAGEMENT = 0xF4,
  LLC_SAP_IBM_REMOTE_PROGRAM_LOAD = 0xF8,
  LLC_SAP_UNGERMANN_BASS = 0xFA,
  LLC_SAP_ISO_NETWORK_LAYER_PROTO = 0xFE,
  LLC_SAP_GLOBAL = 0xFF, // DSAP only
};

/*
 * References:
 *   http://www.wildpackets.com/resources/compendium/llc/type1_commands
 */
enum class LLC_CONTROL : uint16_t {
  // Type I: Connectionless/unreliable
  LLC_CONTROL_UI = 0x03, // Unnumbered Information
  LLC_CONTROL_XID = 0xAF, // Exchange Identification
  LLC_CONTROL_TEST = 0xE3, // Test
};

/*
 * A Logical Link Control Header. The payload is not represented here.
 *
 * TODO: Add support for Type 2 and Type 3 PDUs.
 */
class LlcHdr {
 public:
  /*
   * default constructor
   */
  LlcHdr() {}
  /*
   * copy constructor
   */
  LlcHdr(const LlcHdr& rhs)
      : dsap(rhs.dsap), ssap(rhs.ssap), control(rhs.control) {}
  /*
   * parameterized data constructor
   */
  LlcHdr(uint8_t _dsap, uint8_t _ssap, uint8_t _control)
      : dsap(_dsap), ssap(_ssap), control(_control) {}
  /*
   * cursor data constructor
   */
  explicit LlcHdr(folly::io::Cursor& cursor);
  /*
   * destructor
   */
  ~LlcHdr() {}
  /*
   * operator=
   */
  LlcHdr& operator=(const LlcHdr& rhs) {
    dsap = rhs.dsap;
    ssap = rhs.ssap;
    control = rhs.control;
    return *this;
  }

 public:
  /*
   * Destination service access point address field
   */
  uint8_t dsap{0};
  /*
   * Source service access point address field
   */
  uint8_t ssap{0};
  /*
   * Control field
   */
  uint8_t control{0};
};

inline bool operator==(const LlcHdr& lhs, const LlcHdr& rhs) {
  return lhs.dsap == rhs.dsap && lhs.ssap == rhs.ssap &&
      lhs.control == rhs.control;
}

inline bool operator!=(const LlcHdr& lhs, const LlcHdr& rhs) {
  return !operator==(lhs, rhs);
}

} // namespace facebook::fboss
