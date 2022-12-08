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
 * Internet Control Message Protocol Header for IPv4 and IPv6
 *
 * References:
 *   ICMP: https://tools.ietf.org/html/rfc792
 *   ICMPv6: https://tools.ietf.org/html/rfc4443
 */
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

/**
 * ICMPv4 type and code definition, RFC 792
 */
enum class ICMPv4Type : uint8_t {
  ICMPV4_TYPE_ECHO_REPLY = 0,
  ICMPV4_TYPE_DESTINATION_UNREACHABLE = 3,
  ICMPV4_TYPE_REDIRECT = 5,
  ICMPV4_TYPE_ECHO = 8,
  ICMPV4_TYPE_TIME_EXCEEDED = 11,
  ICMPV4_TYPE_PARAMETER_PROBLEM = 12,
  ICMPV4_TYPE_TIMESTAMP = 13,
  ICMPV4_TYPE_TIMESTAMP_REPLY = 14,
  // NOTE: all deprecated type definitions are removed
};

enum class ICMPv4Code : uint8_t {
  // destination unreachable error code
  ICMPV4_CODE_DESTINATION_UNREACHABLE_NET_UNREACHABLE = 0,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_HOST_UNREACHABLE = 1,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_PROTO_UNREACHABLE = 2,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_PORT_UNREACHABLE = 3,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_FRAGMENTATION_REQUIRED = 4,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_SOURCE_ROUTE_FAILED = 5,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_DESTINATION_NETWORK_UNKNOWN = 6,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_DESTINATION_HOST_UNKNOWN = 7,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_SOURCE_HOST_ISOLATED = 8,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_NET_ADMIN_PROHIBITED = 9,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_HOST_ADMIN_PROHIBITED = 10,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_NET_UNREACHABLE_FORTOS = 11,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_HOST_UNREACHABLE_FORTOS = 12,
  ICMPV4_CODE_DESTINATION_UNREACHABLE_COMM_ADMIN_PROHIBITED = 13,

  // redirect error code
  ICMPV4_CODE_REDIRECT_FOR_NETWORK = 0,
  ICMPV4_CODE_REDIRECT_FOR_HOST = 1,
  ICMPV4_CODE_REDIRECT_FOR_TOS_NETWORK = 2,
  ICMPV4_CODE_REDIRECT_FOR_TOS_HOST = 3,

  // time exceed error code
  ICMPV4_CODE_TIME_EXCEEDED_TTL_EXCEEDED = 0,
  ICMPV4_CODE_TIME_EXCEEDED_FRAGMENT_REASSEMBLY = 1,

  // parameter problem error code
  ICMPV4_CODE_PARAMETER_PROBLEM_POINTER_PROBLEM = 0,
  ICMPV4_CODE_PARAMETER_PROBLEM_MISSING_OPERAND = 1,
  ICMPV4_CODE_PARAMETER_PROBLEM_BAD_LENGTH = 2,
};

/**
 * ICMPv6 type and code definition, RFC 4443
 */
enum class ICMPv6Type : uint8_t {
  // Error Messages: 0-127
  ICMPV6_TYPE_RESERVED_0 = 0,

  ICMPV6_TYPE_DESTINATION_UNREACHABLE = 1,
  ICMPV6_TYPE_PACKET_TOO_BIG = 2,
  ICMPV6_TYPE_TIME_EXCEEDED = 3,
  ICMPV6_TYPE_PARAMETER_PROBLEM = 4,

  ICMPV6_TYPE_PRIVATE_EXPERIMENTATION_100 = 100,
  ICMPV6_TYPE_PRIVATE_EXPERIMENTATION_101 = 101,

  ICMPV6_TYPE_RESERVED_127 = 127,

  // Informational Messages: 128-255

  // Ping
  ICMPV6_TYPE_ECHO_REQUEST = 128,
  ICMPV6_TYPE_ECHO_REPLY = 129,
  ICMPV6_TYPE_MULTICAST_LISTENER_QUERY = 130,
  ICMPV6_TYPE_MULTICAST_LISTENER_REPORT = 131,
  ICMPV6_TYPE_MULTICAST_LISTENER_DONE = 132,

  // NDP: Neighbor Discovery Protocol
  ICMPV6_TYPE_NDP_ROUTER_SOLICITATION = 133,
  ICMPV6_TYPE_NDP_ROUTER_ADVERTISEMENT = 134,
  ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION = 135,
  ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT = 136,
  ICMPV6_TYPE_NDP_REDIRECT_MESSAGE = 137,

  ICMPV6_TYPE_ROUTER_RENUMBERING = 138,
  ICMPV6_TYPE_NODE_INFORMATION_QUERY = 139,
  ICMPV6_TYPE_NODE_INFORMATION_RESPONSE = 140,
  ICMPV6_TYPE_INVERSE_NEIGHBOR_SOLICITATION = 141,
  ICMPV6_TYPE_INVERSE_NEIGHBOR_ADVERTISEMENT = 142,
  ICMPV6_TYPE_MULTICAST_LISTENER_DISCOVERY_REPORTS = 143,

  ICMPV6_TYPE_HOME_AGENT_ADDRESS_DISCOVERY_REQUEST = 144,
  ICMPV6_TYPE_HOME_AGENT_ADDRESS_DISCOVERY_REPLY = 145,
  ICMPV6_TYPE_MOBILE_PREFIX_SOLICITATION = 146,
  ICMPV6_TYPE_MOBILE_PREFIX_ADVERTISEMENT = 147,

  // SEND
  ICMPV6_TYPE_SEND_CERTIFICATION_PATH_SOLICITATION = 148,
  ICMPV6_TYPE_SEND_CERTIFICATION_PATH_ADVERTISEMENT = 149,

  // MRD
  ICMPV6_TYPE_MRD_MULTICAST_ROUTER_ADVERTISEMENT = 151,
  ICMPV6_TYPE_MRD_MULTICAST_ROUTER_SOLICITATION = 152,
  ICMPV6_TYPE_MRD_MULTICAST_ROUTER_TERMINATION = 153,

  ICMPV6_TYPE_RPL_CONTROL_MESSAGE = 155,

  ICMPV6_TYPE_PRIVATE_EXPERIMENTATION_200 = 200,
  ICMPV6_TYPE_PRIVATE_EXPERIMENTATION_201 = 201,
  ICMPV6_TYPE_RESERVED_255 = 255,
};

enum class ICMPv6Code : uint8_t {
  // code for time exceeded messages
  ICMPV6_CODE_TIME_EXCEEDED_HOPLIMIT_EXCEEDED = 0,
  ICMPV6_CODE_TIME_EXCEEDED_FRAG_REASSEMBLE_TIME_EXCCEDED = 1,

  // code for ndp messages
  ICMPV6_CODE_NDP_MESSAGE_CODE = 0,

  // MTU error codes
  ICMPV6_CODE_PACKET_TOO_BIG = 0,

  // parameter problem error code
  ICMPV6_CODE_PARAM_PROBLEM_ERROR_HEADER_FIELD = 0,
  ICMPV6_CODE_PARAM_PROBLEM_BAD_NEXTHEADER_TYPE = 0,
  ICMPV6_CODE_PARAM_PROBLEM_BAD_IPV6_OPTION = 0,

  // others
  ICMPV6_CODE_ECHO_REQUEST = 0,
  ICMPV6_CODE_ECHO_REPLY = 0,
};

enum class ICMPv6ErrorDestinationUnreachable : uint8_t {
  ICMPV6_ERROR_DESTINATION_UNREACHABLE_NO_ROUTE_TO_DESTINATION = 0,
  ICMPV6_ERROR_DESTINATION_UNREACHABLE_COMMUNICATION_PROHIBITED = 1,
  ICMPV6_ERROR_DESTINATION_UNREACHABLE_BEYOND_SCOPE_OF_SOURCE_ADDRESS = 2,
  ICMPV6_ERROR_DESTINATION_UNREACHABLE_ADDRESS_UNREACHABLE = 3,
  ICMPV6_ERROR_DESTINATION_UNREACHABLE_PORT_UNREACHABLE = 4,
  ICMPV6_ERROR_DESTINATION_UNREACHABLE_SOURCE_ADDRESS_FAILED_POLICY = 5,
  ICMPV6_ERROR_DESTINATION_UNREACHABLE_REJECT_ROUTE_TO_DESTINATION = 6,
};

/*
 * An ICMP Header for both IPv4 and IPv6.
 * The payload is not represented here.
 */
class ICMPHdr {
 public:
  enum { SIZE = 4 };
  enum { ICMPV6_UNUSED_LEN = 4 };
  enum { ICMPV6_MTU_LEN = 4 };
  enum { ICMPV4_UNUSED_LEN = 4 };
  enum { ICMPV4_SENDER_BYTES = 8 };
  /*
   * default constructor
   */
  ICMPHdr() {}
  /*
   * copy constructor
   */
  ICMPHdr(const ICMPHdr& rhs)
      : type(rhs.type), code(rhs.code), csum(rhs.csum) {}
  /*
   * parameterized data constructor
   */
  ICMPHdr(uint8_t _type, uint8_t _code, uint16_t _csum)
      : type(_type), code(_code), csum(_csum) {}
  /*
   * cursor data constructor
   */
  explicit ICMPHdr(folly::io::Cursor& cursor);
  /*
   * destructor
   */
  ~ICMPHdr() {}
  /*
   * operator=
   */
  ICMPHdr& operator=(const ICMPHdr& rhs) {
    type = rhs.type;
    code = rhs.code;
    csum = rhs.csum;
    return *this;
  }

  /*
   * Serialize the ICMP header using the specified cursor.
   *
   * The cursor will be updated to point just after the IP header.
   */
  void serialize(folly::io::RWPrivateCursor* cursor) const;

  /*
   * Compute the checksum
   *
   * The Cursor should point to the start of the body data, just after the
   * ICMP header.
   */
  // compute checksum without including ip header, used for icmpv4
  uint16_t computeChecksum(
      const folly::io::Cursor& cursor,
      uint32_t payloadLength) const;
  // compute checksum with pseudo ip header, used for icmpv6
  uint16_t computeChecksum(
      const IPv6Hdr& ipv6Hdr,
      const folly::io::Cursor& cursor) const;

  /**
   * compute the total packet length for ICMPv4 and ICMPv6 given
   * the payload length
   */
  static uint32_t computeTotalLengthV4(uint32_t payloadLength);
  static uint32_t computeTotalLengthV6(uint32_t payloadLength);

  /*
   * Return true if and only if the header checksum is valid.
   */
  bool validateChecksum(const IPv6Hdr& ipv6Hdr, const folly::io::Cursor& cursor)
      const {
    return computeChecksum(ipv6Hdr, cursor) == csum;
  }

  /**
   * serialize the full packet including eth_hdr, ip hdr, icmp_hdr and payload
   */
  // for ICMPv6
  template <typename BodyFn>
  void serializeFullPacket(
      folly::io::RWPrivateCursor* cursor,
      folly::MacAddress dstMac,
      folly::MacAddress srcMac,
      std::optional<VlanID> vlan,
      const IPv6Hdr& ipv6,
      uint32_t payloadLength,
      BodyFn bodyFn) {
    // Write the ethernet and IPv6 header
    serializePktHdr(cursor, dstMac, srcMac, vlan, ipv6, payloadLength);

    // Write the ICMP header, but skip over the checksum for now
    cursor->write<uint8_t>(type);
    cursor->write<uint8_t>(code);
    folly::io::RWPrivateCursor csumCursor(*cursor);
    cursor->skip(2);
    folly::io::Cursor payloadStart(*cursor);

    // Write the payload
    bodyFn(cursor);
    DCHECK((csumCursor + 2 + payloadLength) == *cursor);

    // Now compute and write the checksum
    csum = computeChecksum(ipv6, payloadStart);
    csumCursor.writeBE<uint16_t>(csum);
  }

  template <typename BodyFn>
  void serializeFullPacket(
      folly::io::RWPrivateCursor* cursor,
      folly::MacAddress dstMac,
      folly::MacAddress srcMac,
      std::optional<VlanID> vlan,
      const IPv4Hdr& ipv4,
      uint32_t payloadLength,
      BodyFn bodyFn) {
    // Write the ethernet and IPv4 header
    serializePktHdr(cursor, dstMac, srcMac, vlan, ipv4);

    // Write the ICMP header, but skip over the checksum for now
    cursor->write<uint8_t>(type);
    cursor->write<uint8_t>(code);
    folly::io::RWPrivateCursor csumCursor(*cursor);
    cursor->skip(2);
    folly::io::Cursor payloadStart(*cursor);

    // Write the payload
    bodyFn(cursor);
    DCHECK((csumCursor + 2 + payloadLength) == *cursor);

    // Now compute and write the checksum
    csum = computeChecksum(payloadStart, payloadLength);
    csumCursor.writeBE<uint16_t>(csum);
  }

 private:
  /**
   * helper function to serialize the eth_hdr and ip hdr
   */
  // IPv6 header
  void serializePktHdr(
      folly::io::RWPrivateCursor* cursor,
      folly::MacAddress dstMac,
      folly::MacAddress srcMac,
      std::optional<VlanID> vlan,
      const IPv6Hdr& ipv6,
      uint32_t payloadLength);
  // IPv4 header
  void serializePktHdr(
      folly::io::RWPrivateCursor* cursor,
      folly::MacAddress dstMac,
      folly::MacAddress srcMac,
      std::optional<VlanID> vlan,
      const IPv4Hdr& ipv4);

 public:
  /*
   * Type
   */
  uint8_t type{0};
  /*
   * Code, meaning depends on Type
   */
  uint8_t code{0};
  /*
   * Checksum
   */
  uint16_t csum{0};
};

inline bool operator==(const ICMPHdr& lhs, const ICMPHdr& rhs) {
  return lhs.type == rhs.type && lhs.code == rhs.code && lhs.csum == rhs.csum;
}

inline bool operator!=(const ICMPHdr& lhs, const ICMPHdr& rhs) {
  return !operator==(lhs, rhs);
}

} // namespace facebook::fboss
