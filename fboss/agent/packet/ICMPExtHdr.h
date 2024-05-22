// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/io/Cursor.h>
#include <vector>

/*
 * Internet Control Message Protocol Extended Header for IPv4 and IPv6
 *
 * References:
 *   Extended ICMP to Support Multi-Part messages:  RFC4884
 *   Extending ICMP for Interface and Next-Hop Identification:  RFC5837
 */

namespace facebook::fboss {

// See RFC4884 for the definition of Class-Num uses.
// The values here correspond to IANA registered values found
// in "ICMP Extension Object Classes and Class Sub-types"
enum class ICMPExtObjectClassNum : uint8_t {
  ICMP_EXT_OBJECT_CLASS_MPLS_LABEL_STACK_CLASS = 1,
  ICMP_EXT_OBJECT_CLASS_INTERFACE_INFORMATION_OBJECT = 2,
  ICMP_EXT_OBJECT_CLASS_INTERFACE_IDENTIFICATION_OBJECT = 3,
  ICMP_EXT_OBJECT_CLASS_INTERFACE_EXTENDED_INFORMATION = 4,
  // 5-246 was unassigned at time of writing.
  // 247-255 is reserved for private use.
};

/*
 * An ICMP Extension Object Header.
 * The payload is not represented here.
 * The C-Type value meaning will change depending on the Class-Num
 *
 * See rfc4884, Section 8
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |             Length            |   Class-Num   |   C-Type      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                                                               |
 * |                   // (Object payload) //                      |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
class ICMPExtObjectHdr {
 public:
  enum { SIZE = 4 }; // Header size is only is 4 bytes
  // Length of the object, including payload
  virtual uint16_t length() const = 0;
  ICMPExtObjectClassNum classNum;
  virtual uint8_t cType() const = 0;

  virtual void serialize(folly::io::RWPrivateCursor* cursor);
  virtual ~ICMPExtObjectHdr() {}

 protected:
  ICMPExtObjectHdr() {}
};

/*
 * An ICMP Extension header.
 * Will typically be followed by ICMP Extension Objects
 *
 * See RFC4884, section 7 for format.
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Version|      (Reserved)       |           Checksum            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
class ICMPExtHdr {
 public:
  std::vector<ICMPExtObjectHdr*> objects;
  uint32_t length() const;
  void serialize(folly::io::RWPrivateCursor* cursor);
};

// See RFC 5837 for interface roles on next-hop identification
enum class ICMPExtInterfaceRole : uint8_t {
  // The IP interface upon which a datagram arrived
  ICMP_EXT_INTERFACE_ROLE_DATAGRAM_ARRIVED = 0,

  // sub-ip component of an IP interface upon which a datagram arrived
  ICMP_EXT_INTERFACE_ROLE_SUB_IP_ARRIVED = 1,

  // IP interface through which the datagram would have been forwarded
  // had it been forwardable
  ICMP_EXT_INTERFACE_ROLE_EGRESS_INTERFACE = 2,

  // IP next hop to which the datagram would have been forwarded
  ICMP_EXT_INTERFACE_ROLE_IP_NEXT_HOP = 3,
};

/**
 * See RFC5837, Section 4.1
 * This is a C-Type representation for an Interface Identification Object
  Bit     0       1       2       3       4       5       6       7
      +-------+-------+-------+-------+-------+-------+-------+-------+
      | Interface Role| Rsvd1 | Rsvd2 |ifIndex| IPAddr|  name |  MTU  |
      +-------+-------+-------+-------+-------+-------+-------+-------+
 */
class ICMPExtInterfaceCType {
 public:
  ICMPExtInterfaceRole interfaceRole; // 2 bits
  // next two bits reserved,
  bool ifIndex; // 1 bit
  bool ipAddress; // 1 bit
  bool name; // 1 bit
  bool mtu; // 1 bit

  ICMPExtInterfaceCType() {
    // Set all fields to zero
    interfaceRole =
        ICMPExtInterfaceRole::ICMP_EXT_INTERFACE_ROLE_DATAGRAM_ARRIVED;
    ifIndex = false;
    ipAddress = false;
    name = false;
    mtu = false;
  }

  operator uint8_t() const {
    // Shift interface role across 2, account for reserved, shift across 6 bits
    uint8_t ret = ((uint8_t)interfaceRole) << 6;

    // Add in the other type bits
    ret |= (uint8_t)(ifIndex) << 3;
    ret |= (uint8_t)(ipAddress) << 2;
    ret |= (uint8_t)(name) << 1;
    ret |= (uint8_t)(mtu);

    return ret;
  }
};

/*
* Interface Name Sub-Object
* See RFC5837, Section 4.3

octet    0        1                                   63
      +--------+-----------................-----------------+
      | length |   interface name octets 1-63               |
      +--------+-----------................-----------------+
*/
class ICMPExtIfaceNameSubObject {
 private:
  enum { ICMPExtIfaceNameSubObjectMaxLen = 63 };
  char ifaceName_[ICMPExtIfaceNameSubObjectMaxLen] = {'\0'};

 public:
  ICMPExtIfaceNameSubObject() {}
  explicit ICMPExtIfaceNameSubObject(const char* name);
  void serialize(folly::io::RWPrivateCursor* cursor) const;
  uint32_t length() const;
};

/*
 * Extension header IP Sub Object type.
 */
enum class ICMPExtInterfaceIPSubObjectAFI : uint16_t {
  ICMP_EXT_INTERFACE_IP_SUB_OBJECT_AFI_IPV4 = 1,
  ICMP_EXT_INTERFACE_IP_SUB_OBJECT_AFI_IPV6 = 2,
};

/**
* See RFC5837, Section 4.2
* Represents an IP Address on an interface
    0                            31
    +-------+-------+-------+-------+
    |      AFI      |    Reserved   |
    +-------+-------+-------+-------+
    |         IP Address   ....
*/
class ICMPExtIPSubObject {
 protected:
  ICMPExtIPSubObject() {}

 public:
  virtual ~ICMPExtIPSubObject() {}
  // Length of this object is dependent on AFI
  virtual uint32_t length() const = 0;

  virtual void serialize(folly::io::RWPrivateCursor* cursor) = 0;
};

class ICMPExtIpSubObjectV4 final : public ICMPExtIPSubObject {
 private:
  folly::IPAddressV4 ipAddress_;

 public:
  explicit ICMPExtIpSubObjectV4(folly::IPAddressV4 address)
      : ICMPExtIPSubObject() {
    this->ipAddress_ = address;
  }

  ~ICMPExtIpSubObjectV4() override {}

  uint32_t length() const override {
    // 4 bytes for AFI & Reserved, 4 bytes for IPv4 Address
    return 8;
  }
  void serialize(folly::io::RWPrivateCursor* cursor) override;
};

class ICMPExtIpSubObjectV6 final : public ICMPExtIPSubObject {
 private:
  folly::IPAddressV6 ipAddress_;

 public:
  explicit ICMPExtIpSubObjectV6(folly::IPAddressV6 address)
      : ICMPExtIPSubObject() {
    this->ipAddress_ = address;
  }

  ~ICMPExtIpSubObjectV6() override {}

  uint32_t length() const override {
    // 4 bytes for AFI & Reserved, 16 bytes for IPv6 Address
    return 4 + 16;
  }
  void serialize(folly::io::RWPrivateCursor* cursor) override;
};

/**
 * Interface Information Object.
 * Only IP Objects are supported at the moment
 */
class ICMPExtInterfaceInformationObject : public ICMPExtObjectHdr {
 private:
  std::unique_ptr<ICMPExtIPSubObject> ipObject_;
  std::unique_ptr<ICMPExtIfaceNameSubObject> nameObject_;
  ICMPExtInterfaceCType ifaceCType_;

 public:
  ICMPExtInterfaceInformationObject(
      std::unique_ptr<ICMPExtIPSubObject> ipObj,
      std::unique_ptr<ICMPExtIfaceNameSubObject> nameObj)
      : ICMPExtObjectHdr() {
    classNum = ICMPExtObjectClassNum::
        ICMP_EXT_OBJECT_CLASS_INTERFACE_INFORMATION_OBJECT;

    if (ipObj != nullptr) {
      this->ipObject_ = std::move(ipObj);
      this->ifaceCType_.ipAddress = true;
    }

    if (nameObj != nullptr) {
      this->nameObject_ = std::move(nameObj);
      this->ifaceCType_.name = true;
    }
  }
  ~ICMPExtInterfaceInformationObject() override {}
  uint16_t length() const override; // length of this object, including payload
  void serialize(folly::io::RWPrivateCursor* cursor) override;
  uint8_t cType() const override {
    return static_cast<uint8_t>(ifaceCType_);
  }
};

} // namespace facebook::fboss
