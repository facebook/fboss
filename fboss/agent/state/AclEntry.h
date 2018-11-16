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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/MatchAction.h"

#include <string>
#include <utility>
#include <folly/IPAddress.h>
#include <folly/Optional.h>
#include <folly/MacAddress.h>

namespace facebook { namespace fboss {

template<typename T>
class GenericAclRange {
public:
 GenericAclRange(const T& min, const T& max, bool invert)
     : min_(min), max_(max), invert_(invert) {}
 GenericAclRange(const GenericAclRange& range)
     : GenericAclRange(range.min_, range.max_, range.invert_) {}
 T getMin() const {
   return min_;
  }
  void setMin(const T min) {
    min_ = min;
  }
  T getMax() const {
    return max_;
  }
  void setMax(const T max) {
    max_ = max;
  }
  void setInvert(const bool invert) {
    invert_ = invert;
  }
  bool getInvert() const {
    return invert_;
  }
  bool isExactMatch() const {
    return !invert_ && min_ == max_;
  }
  bool operator<(const GenericAclRange& r) const {
    return std::tie(min_, max_, invert_) < std::tie(r.min_, r.max_, r.invert_);
  }
  bool operator==(const GenericAclRange& r) const {
    return std::tie(min_, max_, invert_) == std::tie(r.min_, r.max_, r.invert_);
  }
  GenericAclRange& operator=(const GenericAclRange& r) {
    min_ = r.min_;
    max_ = r.max_;
    invert_ = r.invert_;
    return *this;
  }
  folly::dynamic toFollyDynamic() const {
    folly::dynamic range = folly::dynamic::object;
    range[kAclRangeMin] = static_cast<T>(min_);
    range[kAclRangeMax] = static_cast<T>(max_);
    range[kAclRangeInvert] = static_cast<bool>(invert_);
    return range;
  }
  static T getLowerLimit() {
    return std::numeric_limits<T>::min();
  }
  static T getUpperLimit() {
    return std::numeric_limits<T>::max();
  }
  static GenericAclRange fromFollyDynamic(const folly::dynamic& rangeJson) {
    checkFollyDynamic(rangeJson);
    T min = rangeJson[kAclRangeMin].asInt();
    T max = rangeJson[kAclRangeMax].asInt();
    bool invert = rangeJson[kAclRangeInvert].asBool();
    return GenericAclRange(min, max, invert);
  }
  static void checkFollyDynamic(const folly::dynamic& rangeJson) {
    if (rangeJson.find(kAclRangeMin) == rangeJson.items().end()) {
      throw FbossError("a range should have min value set");
    }
    if (rangeJson.find(kAclRangeMin) == rangeJson.items().end()) {
      throw FbossError("a range should have max value set");
    }
    T pMin = rangeJson[kAclRangeMin].asInt();
    T pMax = rangeJson[kAclRangeMax].asInt();
    if (pMin > getUpperLimit() || pMax > getUpperLimit()) {
      throw FbossError("Range value exceeds ", std::to_string(getUpperLimit()));
    }
    if (pMin < getLowerLimit() || pMax < getLowerLimit()) {
      throw FbossError("Range value lower than ",
        std::to_string(getLowerLimit()));
    }
    if (pMin > pMax) {
      throw FbossError("Min. value is larger than max. value");
    }
  }

protected:
  T min_;
  T max_;
  bool invert_;
  static constexpr auto kAclRangeMin = "min";
  static constexpr auto kAclRangeMax = "max";
  static constexpr auto kAclRangeInvert = "invert";
};

using AclL4PortRange = GenericAclRange<uint16_t>;
using AclPktLenRange = GenericAclRange<uint16_t>;

class AclTtl {
public:
  AclTtl(const AclTtl& ttl) {
    setValue(ttl.value_);
    setMask(ttl.mask_);
  }

  AclTtl(const uint16_t value, const uint16_t mask) {
    setValue(value);
    setMask(mask);
  }

  uint16_t getValue() const {
    return value_;
  }

  void setValue(uint16_t value) {
    if (value > 255) {
      throw FbossError("ttl value is invalid (must be [0,255])");
    }
    value_ = value;
  }

  uint16_t getMask() const {
    return mask_;
  }

  void setMask(uint16_t mask) {
    if (mask > 255) {
      throw FbossError("ttl mask is invalid (must be [0,255])");
    }
    mask_ = mask;
  }

  bool operator<(const AclTtl& ttl) const {
    return std::tie(value_, mask_) < std::tie(ttl.value_, ttl.mask_);
  }

  bool operator==(const AclTtl& ttl) const {
    return std::tie(value_, mask_) == std::tie(ttl.value_, ttl.mask_);
  }

  AclTtl& operator=(const AclTtl& ttl) {
    value_ = ttl.value_;
    mask_ = ttl.mask_;
    return *this;
  }

  folly::dynamic toFollyDynamic() const;
  static AclTtl fromFollyDynamic(const folly::dynamic& rangeJson);
private:
  uint16_t value_;
  uint16_t mask_;
};

struct AclEntryFields {
  static const uint8_t kProtoIcmp = 1;
  static const uint8_t kProtoIcmpv6 = 58;
  static const uint8_t kMaxIcmpType = 0xFF;
  static const uint8_t kMaxIcmpCode = 0xFF;
  explicit AclEntryFields(int priority, const std::string& name)
    : priority(priority),
      name(name) {}

  template<typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static AclEntryFields fromFollyDynamic(const folly::dynamic& json);
  static void checkFollyDynamic(const folly::dynamic& json);
  static void checkFollyDynamicPortRange(const folly::dynamic& json);
  int priority{0};
  std::string name{nullptr};
  folly::CIDRNetwork srcIp{std::make_pair(folly::IPAddress(), 0)};
  folly::CIDRNetwork dstIp{std::make_pair(folly::IPAddress(), 0)};
  folly::Optional<uint8_t> proto{folly::none};
  folly::Optional<uint8_t> tcpFlagsBitMap{folly::none};
  folly::Optional<uint16_t> srcPort{folly::none};
  folly::Optional<uint16_t> dstPort{folly::none};
  folly::Optional<AclL4PortRange> srcL4PortRange{folly::none};
  folly::Optional<AclL4PortRange> dstL4PortRange{folly::none};
  folly::Optional<AclPktLenRange> pktLenRange{folly::none};
  folly::Optional<cfg::IpFragMatch> ipFrag{folly::none};
  folly::Optional<uint8_t> icmpType{folly::none};
  folly::Optional<uint8_t> icmpCode{folly::none};
  folly::Optional<uint8_t> dscp{folly::none};
  folly::Optional<cfg::IpType> ipType{folly::none};
  folly::Optional<AclTtl> ttl{folly::none};
  folly::Optional<folly::MacAddress> dstMac{folly::none};

  cfg::AclActionType actionType{cfg::AclActionType::PERMIT};
  folly::Optional<MatchAction> aclAction{folly::none};
};

/*
 * AclEntry stores state about one of the access control entries on
 * the switch.
 */
class AclEntry :
    public NodeBaseT<AclEntry, AclEntryFields> {
 public:
  explicit AclEntry(int priority, const std::string& name);
  static std::shared_ptr<AclEntry>
  fromFollyDynamic(const folly::dynamic& json) {
    const auto& fields = AclEntryFields::fromFollyDynamic(json);
    return std::make_shared<AclEntry>(fields);
  }

  static std::shared_ptr<AclEntry>
  fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  bool operator==(const AclEntry& acl) const {
    return getFields()->priority == acl.getPriority() &&
           getFields()->name == acl.getID() &&
           getFields()->actionType == acl.getActionType() &&
           getFields()->aclAction == acl.getAclAction() &&
           getFields()->srcIp == acl.getSrcIp() &&
           getFields()->dstIp == acl.getDstIp() &&
           getFields()->proto == acl.getProto() &&
           getFields()->tcpFlagsBitMap == acl.getTcpFlagsBitMap() &&
           getFields()->srcPort == acl.getSrcPort() &&
           getFields()->dstPort == acl.getDstPort() &&
           getFields()->srcL4PortRange == acl.getSrcL4PortRange() &&
           getFields()->dstL4PortRange == acl.getDstL4PortRange() &&
           getFields()->pktLenRange == acl.getPktLenRange() &&
           getFields()->ipFrag == acl.getIpFrag() &&
           getFields()->icmpType == acl.getIcmpType() &&
           getFields()->icmpCode == acl.getIcmpCode() &&
           getFields()->dscp == acl.getDscp() &&
           getFields()->dstMac == acl.getDstMac() &&
           getFields()->ipType == acl.getIpType() &&
           getFields()->ttl == acl.getTtl();
  }

  int getPriority() const {
    return getFields()->priority;
  }

  const std::string& getID() const {
    return getFields()->name;
  }

  const folly::Optional<MatchAction> getAclAction() const {
    return getFields()->aclAction;
  }

  void setAclAction(const MatchAction& action) {
    writableFields()->aclAction = action;
  }

  cfg::AclActionType getActionType() const {
    return getFields()->actionType;
  }

  void setActionType(const cfg::AclActionType& actionType) {
    writableFields()->actionType = actionType;
  }

  folly::CIDRNetwork getSrcIp() const {
    return getFields()->srcIp;
  }

  void setSrcIp(const folly::CIDRNetwork& ip) {
    writableFields()->srcIp = ip;
  }

  folly::CIDRNetwork getDstIp() const {
    return getFields()->dstIp;
  }

  void setDstIp(const folly::CIDRNetwork& ip) {
    writableFields()->dstIp = ip;
  }

  folly::Optional<uint8_t> getProto() const {
    return getFields()->proto;
  }

  void setProto(const uint8_t proto) {
    writableFields()->proto = proto;
  }

  folly::Optional<uint8_t> getTcpFlagsBitMap() const {
    return getFields()->tcpFlagsBitMap;
  }

  void setTcpFlagsBitMap(const uint8_t flagsBitMap) {
    writableFields()->tcpFlagsBitMap = flagsBitMap;
  }

  folly::Optional<uint16_t> getSrcPort() const {
    return getFields()->srcPort;
  }

  void setSrcPort(const uint16_t port) {
    writableFields()->srcPort = port;
  }

  folly::Optional<uint16_t> getDstPort() const {
    return getFields()->dstPort;
  }

  void setDstPort(const uint16_t port) {
    writableFields()->dstPort = port;
  }

  void setSrcL4PortRange(const AclL4PortRange& r) {
    writableFields()->srcL4PortRange = r;
  }

  folly::Optional<AclL4PortRange> getSrcL4PortRange() const {
    return getFields()->srcL4PortRange;
  }

  void setDstL4PortRange(const AclL4PortRange& r) {
    writableFields()->dstL4PortRange = r;
  }

  folly::Optional<AclL4PortRange> getDstL4PortRange() const {
    return getFields()->dstL4PortRange;
  }

  folly::Optional<AclPktLenRange> getPktLenRange() const {
    return getFields()->pktLenRange;
  }

  void setPktLenRange(const AclPktLenRange& r) {
    writableFields()->pktLenRange = r;
  }

  folly::Optional<cfg::IpFragMatch> getIpFrag() const {
    return getFields()->ipFrag;
  }

  void setIpFrag(const cfg::IpFragMatch& frag) {
    writableFields()->ipFrag = frag;
  }

  folly::Optional<uint8_t> getIcmpCode() const {
    return getFields()->icmpCode;
  }

  void setIcmpCode(const uint8_t code) {
    writableFields()->icmpCode = code;
  }

  folly::Optional<uint8_t> getIcmpType() const {
    return getFields()->icmpType;
  }

  void setIcmpType(const uint8_t type) {
    writableFields()->icmpType = type;
  }

  folly::Optional<uint8_t> getDscp() const {
    return getFields()->dscp;
  }

  void setDscp(uint8_t dscp) {
    writableFields()->dscp = dscp;
  }

  folly::Optional<cfg::IpType> getIpType() const {
    return getFields()->ipType;
  }

  void setIpType(const cfg::IpType& ipType) {
    writableFields()->ipType = ipType;
  }

  folly::Optional<AclTtl> getTtl() const {
    return getFields()->ttl;
  }

  void setTtl(const AclTtl& ttl) {
    writableFields()->ttl = ttl;
  }

  folly::Optional<folly::MacAddress> getDstMac() const {
    return getFields()->dstMac;
  }

  void setDstMac(const folly::MacAddress& dstMac) {
    writableFields()->dstMac = dstMac;
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

}} // facebook::fboss
