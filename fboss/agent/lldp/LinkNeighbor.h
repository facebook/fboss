// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include "fboss/agent/types.h"

#include <chrono>
#include <folly/MacAddress.h>
#include <folly/Range.h>

namespace folly { namespace io {
class Cursor;
}}

namespace facebook { namespace fboss {

enum class LinkProtocol {
  UNKNOWN,
  LLDP,
  CDP,
};

enum class LldpChassisIdType : uint8_t {
  RESERVED = 0,
  CHASSIS_ALIAS = 1,
  INTERFACE_ALIAS = 2,
  PORT_ALIAS = 3,
  MAC_ADDRESS = 4,
  NET_ADDRESS = 5,
  INTERFACE_NAME = 6,
  LOCALLY_ASSIGNED = 7,
};

enum class LldpPortIdType : uint8_t {
  RESERVED = 0,
  INTERFACE_ALIAS = 1,
  PORT_ALIAS = 2,
  MAC_ADDRESS = 3,
  NET_ADDRESS = 4,
  INTERFACE_NAME = 5,
  AGENT_CIRCUIT_ID = 6,
  LOCALLY_ASSIGNED = 7,
};

/*
 * LinkNeighbor stores information about an LLDP or CDP neighbor.
 */
struct LinkNeighbor {
 public:
  LinkNeighbor();

  /*
   * Return the protocol over which this neighbor was seen.
   */
  LinkProtocol getProtocol() const {
    return protocol_;
  }

  /*
   * Get the local port on which this neighbor was seen.
   */
  PortID getLocalPort() const {
    return localPort_;
  }

  /*
   * Get the VLAN on which this neighbor was seen.
   */
  VlanID getLocalVlan() const {
    return localVlan_;
  }

  /*
   * Get this neighbor's source MAC address.
   */
  folly::MacAddress getMac() const {
    return srcMac_;
  }

  LldpChassisIdType getChassisIdType() const {
    return chassisIdType_;
  }

  /*
   * Get the chassis ID.
   *
   * The format of the returned string is indicated by getChassisIdType(),
   * and may contain binary data.
   *
   * humanReadableChassisId() returns the chassis ID in a human readable
   * format.
   */
  const std::string& getChassisId() const {
    return chassisId_;
  }

  LldpPortIdType getPortIdType() const {
    return portIdType_;
  }

  /*
   * Get the port ID.
   *
   * The format of the returned string is indicated by getPortIdType(),
   * and may contain binary data.
   *
   * humanReadablePortId() returns the port ID in a human readable
   * format.
   */
  const std::string& getPortId() const {
    return portId_;
  }

  /*
   * Get the capabilities bitmap.
   */
  uint16_t getCapabilities() const {
    return capabilities_;
  }

  /*
   * Get the enabled capabilities bitmap.
   */
  uint16_t getEnabledCapabilities() const {
    return enabledCapabilities_;
  }

  /*
   * Get the system name.
   *
   * This may be empty if it was not specified by the neighbor.
   */
  const std::string& getSystemName() const {
    return systemName_;
  }

  /*
   * Get the neighbor's port description.
   *
   * This may be empty if it was not specified by the neighbor.
   */
  const std::string& getPortDescription() const {
    return portDescription_;
  }

  /*
   * Get the system description.
   *
   * This may be empty if it was not specified by the neighbor.
   */
  const std::string& getSystemDescription() const {
    return systemDescription_;
  }

  /*
   * Get the TTL originally specified in the packet.
   *
   * This always returns the value contained in the packet.  Use
   * getExpirationTime() if you want to check how much time is left until the
   * neighbor information expires.
   */
  std::chrono::seconds getTTL() const {
    return receivedTTL_;
  }

  /*
   * Get the time at which this neighbor information expires.
   */
  std::chrono::steady_clock::time_point getExpirationTime() const {
    return expirationTime_;
  }

  /*
   * Get a human readable representation of the chassis ID.
   *
   * This examines the chassis ID type and converts binary formats (such as
   * binary MAC addresses and IP addresses) into human-readable strings.  If
   * the chassis ID has binary data which it doesn't know how to parse
   * it simply escapse non-printable characters using folly::humanify().
   */
  std::string humanReadableChassisId() const;

  /*
   * Get a human readable representation of the port ID.
   */
  std::string humanReadablePortId() const;

  /*
   * Set neighbor fields based on an LLDP packet.
   *
   * This only updates fields present in the LLDP packet, and does not clear
   * other fields.  This should only be called on a newly constructed
   * LinkNeighbor object.
   *
   * Returns true if this was a valid LLDP packet and the fields were updated
   * successfully, or false if the packet could not be parsed successfully.
   *
   * On error the LinkNeighbor object may be in an undefined state, and should
   * be destroyed or reset with reset().
   */
  bool parseLldpPdu(PortID srcPort,
                    VlanID vlan,
                    folly::MacAddress srcMac,
                    uint16_t ethertype,
                    folly::io::Cursor* cursor);

  /*
   * Set neighbor fields based on a CDP packet.
   *
   * This only updates fields present in the CDP packet, and does not clear
   * other fields.  This should only be called on a newly constructed
   * LinkNeighbor object.
   *
   * Returns true if this was a valid CDP packet and the fields were updated
   * successfully, or false if the packet could not be parsed successfully.
   *
   * On error the LinkNeighbor object may be in an undefined state, and should
   * be destroyed or reset with reset().
   */
  bool parseCdpPdu(PortID srcPort,
                   VlanID vlan,
                   folly::MacAddress srcMac,
                   uint16_t ethertype,
                   folly::io::Cursor* cursor);

 private:
  enum class LldpTlvType : uint8_t;
  enum class CdpTlvType : uint16_t;

  // Private methods
  static std::string humanReadableMac(const std::string& data);
  static std::string humanReadableNetAddr(const std::string& data);

  void setTTL(std::chrono::seconds seconds);

  LldpTlvType parseLldpTlv(folly::io::Cursor* cursor);
  void parseLldpChassis(folly::io::Cursor* cursor, uint16_t length);
  void parseLldpPortId(folly::io::Cursor* cursor, uint16_t length);
  void parseLldpTtl(folly::io::Cursor* cursor, uint16_t length);
  void parseLldpSystemCaps(folly::io::Cursor* cursor, uint16_t length);

  bool parseCdpPayload(PortID srcPort,
                       folly::MacAddress srcMac,
                       folly::io::Cursor* cursor);

  // Data members
  LinkProtocol protocol_{LinkProtocol::UNKNOWN};
  PortID localPort_{0};
  VlanID localVlan_{0};
  folly::MacAddress srcMac_;

  LldpChassisIdType chassisIdType_{LldpChassisIdType::RESERVED};
  LldpPortIdType portIdType_{LldpPortIdType::RESERVED};
  uint16_t capabilities_{0};
  uint16_t enabledCapabilities_{0};

  std::string chassisId_;
  std::string portId_;
  std::string systemName_;
  std::string portDescription_;
  std::string systemDescription_;

  std::chrono::seconds receivedTTL_;
  std::chrono::steady_clock::time_point expirationTime_;
};

}} // facebook::fboss
