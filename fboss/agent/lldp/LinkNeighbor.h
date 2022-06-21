// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include "fboss/agent/lldp/Lldp.h"
#include "fboss/agent/lldp/gen-cpp2/lldp_types.h"
#include "fboss/agent/types.h"
#include "folly/MacAddress.h"

#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <chrono>

namespace folly {
namespace io {
class Cursor;
}
} // namespace folly

namespace facebook::fboss {

/*
 * LinkNeighbor stores information about an LLDP or CDP neighbor.
 */
struct LinkNeighbor {
 public:
  LinkNeighbor();

  /*
   * Return the protocol over which this neighbor was seen.
   */
  lldp::LinkProtocol getProtocol() const {
    return *fields_.protocol();
  }

  /*
   * Get the local port on which this neighbor was seen.
   */
  PortID getLocalPort() const {
    return static_cast<PortID>(*fields_.localPort());
  }

  /*
   * Get the VLAN on which this neighbor was seen.
   */
  VlanID getLocalVlan() const {
    return static_cast<VlanID>(*fields_.localVlan());
  }

  /*
   * Get this neighbor's source MAC address.
   */
  folly::MacAddress getMac() const {
    return folly::MacAddress::fromNBO(*fields_.srcMac());
  }

  lldp::LldpChassisIdType getChassisIdType() const {
    return *fields_.chassisIdType();
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
    return *fields_.chassisId();
  }

  lldp::LldpPortIdType getPortIdType() const {
    return *fields_.portIdType();
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
    return *fields_.portId();
  }

  /*
   * Get the capabilities bitmap.
   */
  uint16_t getCapabilities() const {
    return *fields_.capabilities();
  }

  /*
   * Get the enabled capabilities bitmap.
   */
  uint16_t getEnabledCapabilities() const {
    return *fields_.enabledCapabilities();
  }

  /*
   * Get the system name.
   *
   * This may be empty if it was not specified by the neighbor.
   */
  const std::string& getSystemName() const {
    return *fields_.systemName();
  }

  /*
   * Get the neighbor's port description.
   *
   * This may be empty if it was not specified by the neighbor.
   */
  const std::string& getPortDescription() const {
    return *fields_.portDescription();
  }

  /*
   * Get the system description.
   *
   * This may be empty if it was not specified by the neighbor.
   */
  const std::string& getSystemDescription() const {
    return *fields_.systemDescription();
  }

  /*
   * Get the TTL originally specified in the packet.
   *
   * This always returns the value contained in the packet.  Use
   * getExpirationTime() if you want to check how much time is left until the
   * neighbor information expires.
   */
  std::chrono::seconds getTTL() const {
    return std::chrono::seconds(*fields_.receivedTTL());
  }

  /*
   * Get the time at which this neighbor information expires.
   */
  std::chrono::steady_clock::time_point getExpirationTime() const {
    return std::chrono::steady_clock::time_point(
        std::chrono::seconds(*fields_.expirationTime()));
  }
  bool isExpired(std::chrono::steady_clock::time_point now) const {
    return now > getExpirationTime();
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
  bool parseLldpPdu(
      PortID srcPort,
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
  bool parseCdpPdu(
      PortID srcPort,
      VlanID vlan,
      folly::MacAddress srcMac,
      uint16_t ethertype,
      folly::io::Cursor* cursor);

  /*
   * Functions for updating LinkNeighbor fields.
   * These are mostly useful for testing purposes.
   */
  void setProtocol(lldp::LinkProtocol protocol) {
    fields_.protocol() = protocol;
  }
  void setLocalPort(PortID port) {
    fields_.localPort() = port;
  }
  void setLocalVlan(VlanID vlan) {
    fields_.localVlan() = vlan;
  }
  void setMac(folly::MacAddress mac) {
    fields_.srcMac() = mac.u64NBO();
  }

  void setChassisId(folly::StringPiece id, lldp::LldpChassisIdType type) {
    fields_.chassisIdType() = type;
    fields_.chassisId() = id.str();
  }

  void setPortId(folly::StringPiece id, lldp::LldpPortIdType type) {
    fields_.portIdType() = type;
    fields_.portId() = id.str();
  }
  void setCapabilities(uint16_t caps) {
    fields_.capabilities() = caps;
  }
  void setEnabledCapabilities(uint16_t caps) {
    fields_.enabledCapabilities() = caps;
  }
  void setSystemName(folly::StringPiece name) {
    fields_.systemName() = name.str();
  }
  void setPortDescription(folly::StringPiece desc) {
    fields_.portDescription() = desc.str();
  }
  void setSystemDescription(folly::StringPiece desc) {
    fields_.systemDescription() = desc.str();
  }

  /*
   * Set the TTL.
   *
   * This also updates the expiration time to be the current time plus the TTL.
   */
  void setTTL(std::chrono::seconds seconds) {
    setTTL(seconds, std::chrono::steady_clock::now() + seconds);
  }

  /*
   * Set the TTL and also explicitly set the expiration time.
   */
  void setTTL(
      std::chrono::seconds seconds,
      std::chrono::steady_clock::time_point expiration) {
    fields_.receivedTTL() = seconds.count();
    setExpirationTime(expiration);
  }

  /*
   * LinkNeighbor is a value type, and can be copied around and
   * used directly on the stack.  Therefore we intentionally support
   * copy and move operations.
   */
  LinkNeighbor& operator=(const LinkNeighbor&) = default;
  LinkNeighbor& operator=(LinkNeighbor&&) = default;
  LinkNeighbor(const LinkNeighbor&) = default;
  LinkNeighbor(LinkNeighbor&&) = default;

 private:
  enum class LldpTlvType : uint8_t;
  enum class CdpTlvType : uint16_t;

  // Private methods
  static std::string humanReadableMac(const std::string& data);
  static std::string humanReadableNetAddr(const std::string& data);

  LldpTlvType parseLldpTlv(folly::io::Cursor* cursor);
  void parseLldpChassis(folly::io::Cursor* cursor, uint16_t length);
  void parseLldpPortId(folly::io::Cursor* cursor, uint16_t length);
  void parseLldpTtl(folly::io::Cursor* cursor, uint16_t length);
  void parseLldpSystemCaps(folly::io::Cursor* cursor, uint16_t length);

  bool parseCdpPayload(
      PortID srcPort,
      folly::MacAddress srcMac,
      folly::io::Cursor* cursor);

  void setExpirationTime(std::chrono::steady_clock::time_point expiration) {
    fields_.expirationTime() = std::chrono::duration_cast<std::chrono::seconds>(
                                   expiration.time_since_epoch())
                                   .count();
  }

  lldp::LinkNeighborFields fields_;
};

} // namespace facebook::fboss
