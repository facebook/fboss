// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include "fboss/agent/lldp/Lldp.h"
#include "fboss/agent/lldp/lldp_thrift_types.h"
#include "fboss/agent/types.h"
#include "fboss/thrift_cow/nodes/Types.h"
#include "folly/MacAddress.h"

#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <chrono>

namespace folly::io {
class Cursor;
} // namespace folly::io

namespace facebook::fboss {

/*
 * LinkNeighbor stores information about an LLDP or CDP neighbor.
 */
ADD_THRIFT_RESOLVER_MAPPING(lldp::LinkNeighborFields, LinkNeighbor);
class LinkNeighbor
    : public thrift_cow::ThriftStructNode<lldp::LinkNeighborFields> {
 public:
  using BaseT = ThriftStructNode<lldp::LinkNeighborFields>;
  using BaseT::BaseT;

  LinkNeighbor();

  /*
   * Return the protocol over which this neighbor was seen.
   */
  lldp::LinkProtocol getProtocol() const {
    return cref<lldp_tags::protocol>()->cref();
  }

  /*
   * Get the local port on which this neighbor was seen.
   */
  PortID getLocalPort() const {
    return static_cast<PortID>(cref<lldp_tags::localPort>()->cref());
  }

  /*
   * Get the VLAN on which this neighbor was seen.
   */
  VlanID getLocalVlan() const {
    return static_cast<VlanID>(cref<lldp_tags::localVlan>()->cref());
  }

  /*
   * Get this neighbor's source MAC address.
   */
  folly::MacAddress getMac() const {
    return folly::MacAddress::fromNBO(cref<lldp_tags::srcMac>()->cref());
  }

  lldp::LldpChassisIdType getChassisIdType() const {
    return cref<lldp_tags::chassisIdType>()->cref();
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
    return cref<lldp_tags::chassisId>()->cref();
  }

  lldp::LldpPortIdType getPortIdType() const {
    return cref<lldp_tags::portIdType>()->cref();
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
    return cref<lldp_tags::portId>()->cref();
  }

  /*
   * Get the capabilities bitmap.
   */
  uint16_t getCapabilities() const {
    return cref<lldp_tags::capabilities>()->cref();
  }

  /*
   * Get the enabled capabilities bitmap.
   */
  uint16_t getEnabledCapabilities() const {
    return cref<lldp_tags::enabledCapabilities>()->cref();
  }

  /*
   * Get the system name.
   *
   * This may be empty if it was not specified by the neighbor.
   */
  const std::string& getSystemName() const {
    return cref<lldp_tags::systemName>()->cref();
  }

  /*
   * Get the neighbor's port description.
   *
   * This may be empty if it was not specified by the neighbor.
   */
  const std::string& getPortDescription() const {
    return cref<lldp_tags::portDescription>()->cref();
  }

  /*
   * Get the system description.
   *
   * This may be empty if it was not specified by the neighbor.
   */
  const std::string& getSystemDescription() const {
    return cref<lldp_tags::systemDescription>()->cref();
  }

  /*
   * Get the TTL originally specified in the packet.
   *
   * This always returns the value contained in the packet.  Use
   * getExpirationTime() if you want to check how much time is left until the
   * neighbor information expires.
   */
  std::chrono::seconds getTTL() const {
    return std::chrono::seconds(cref<lldp_tags::receivedTTL>()->cref());
  }

  /*
   * Get the time at which this neighbor information expires.
   */
  std::chrono::steady_clock::time_point getExpirationTime() const {
    return std::chrono::steady_clock::time_point(
        std::chrono::seconds(cref<lldp_tags::expirationTime>()->cref()));
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
      std::optional<VlanID> vlanID,
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
      std::optional<VlanID> vlanID,
      folly::MacAddress srcMac,
      uint16_t ethertype,
      folly::io::Cursor* cursor);

  /*
   * Functions for updating LinkNeighbor fields.
   * These are mostly useful for testing purposes.
   */
  void setProtocol(lldp::LinkProtocol protocol) {
    ref<lldp_tags::protocol>() = protocol;
  }
  void setLocalPort(PortID port) {
    ref<lldp_tags::localPort>() = port;
  }
  void setLocalVlan(VlanID vlan) {
    ref<lldp_tags::localVlan>() = vlan;
  }
  void setMac(folly::MacAddress mac) {
    ref<lldp_tags::srcMac>() = mac.u64NBO();
  }

  void setChassisId(folly::StringPiece id, lldp::LldpChassisIdType type) {
    ref<lldp_tags::chassisIdType>() = type;
    ref<lldp_tags::chassisId>() = id.str();
  }

  void setPortId(folly::StringPiece id, lldp::LldpPortIdType type) {
    ref<lldp_tags::portIdType>() = type;
    ref<lldp_tags::portId>() = id.str();
  }
  void setCapabilities(uint16_t caps) {
    ref<lldp_tags::capabilities>() = caps;
  }
  void setEnabledCapabilities(uint16_t caps) {
    ref<lldp_tags::enabledCapabilities>() = caps;
  }
  void setSystemName(folly::StringPiece name) {
    ref<lldp_tags::systemName>() = name.str();
  }
  void setPortDescription(folly::StringPiece desc) {
    ref<lldp_tags::portDescription>() = desc.str();
  }
  void setSystemDescription(folly::StringPiece desc) {
    ref<lldp_tags::systemDescription>() = desc.str();
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
    ref<lldp_tags::receivedTTL>() = seconds.count();
    setExpirationTime(expiration);
  }

  std::string getKey() const {
    return fmt::format(
        "{}_{}_{}_{}",
        getPortId(),
        getChassisId(),
        getPortIdType(),
        getChassisIdType());
  }

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
    ref<lldp_tags::expirationTime>() =
        std::chrono::duration_cast<std::chrono::seconds>(
            expiration.time_since_epoch())
            .count();
  }

  friend class CloneAllocator;
};

} // namespace facebook::fboss
