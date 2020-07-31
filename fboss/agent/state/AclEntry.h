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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/MatchAction.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <optional>
#include <string>
#include <utility>

namespace facebook::fboss {

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
  static AclTtl fromFollyDynamic(const folly::dynamic& ttlJson);

 private:
  uint16_t value_;
  uint16_t mask_;
};

struct AclEntryFields {
  static const uint8_t kProtoIcmp = 1;
  static const uint8_t kProtoIcmpv6 = 58;
  static const uint8_t kMaxIcmpType = 0xFF;
  static const uint8_t kMaxIcmpCode = 0xFF;
  static const uint16_t kMaxL4Port = 65535;

  explicit AclEntryFields(int priority, const std::string& name)
      : priority(priority), name(name) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static AclEntryFields fromFollyDynamic(const folly::dynamic& json);
  static void checkFollyDynamic(const folly::dynamic& json);
  int priority{0};
  std::string name{nullptr};
  folly::CIDRNetwork srcIp{std::make_pair(folly::IPAddress(), 0)};
  folly::CIDRNetwork dstIp{std::make_pair(folly::IPAddress(), 0)};
  std::optional<uint8_t> proto{std::nullopt};
  std::optional<uint8_t> tcpFlagsBitMap{std::nullopt};
  std::optional<uint16_t> srcPort{std::nullopt};
  std::optional<uint16_t> dstPort{std::nullopt};
  std::optional<cfg::IpFragMatch> ipFrag{std::nullopt};
  std::optional<uint8_t> icmpType{std::nullopt};
  std::optional<uint8_t> icmpCode{std::nullopt};
  std::optional<uint8_t> dscp{std::nullopt};
  std::optional<cfg::IpType> ipType{std::nullopt};
  std::optional<AclTtl> ttl{std::nullopt};
  std::optional<folly::MacAddress> dstMac{std::nullopt};
  std::optional<uint16_t> l4SrcPort{std::nullopt};
  std::optional<uint16_t> l4DstPort{std::nullopt};
  std::optional<cfg::AclLookupClass> lookupClassL2{std::nullopt};
  std::optional<cfg::AclLookupClass> lookupClass{std::nullopt};

  cfg::AclActionType actionType{cfg::AclActionType::PERMIT};
  std::optional<MatchAction> aclAction{std::nullopt};
};

/*
 * AclEntry stores state about one of the access control entries on
 * the switch.
 */
class AclEntry : public NodeBaseT<AclEntry, AclEntryFields> {
 public:
  explicit AclEntry(int priority, const std::string& name);
  static std::shared_ptr<AclEntry> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = AclEntryFields::fromFollyDynamic(json);
    return std::make_shared<AclEntry>(fields);
  }

  static std::shared_ptr<AclEntry> fromJson(const folly::fbstring& jsonStr) {
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
        getFields()->ipFrag == acl.getIpFrag() &&
        getFields()->icmpType == acl.getIcmpType() &&
        getFields()->icmpCode == acl.getIcmpCode() &&
        getFields()->dscp == acl.getDscp() &&
        getFields()->dstMac == acl.getDstMac() &&
        getFields()->ipType == acl.getIpType() &&
        getFields()->ttl == acl.getTtl() &&
        getFields()->l4SrcPort == acl.getL4SrcPort() &&
        getFields()->l4DstPort == acl.getL4DstPort() &&
        getFields()->lookupClassL2 == acl.getLookupClassL2() &&
        getFields()->lookupClass == acl.getLookupClass();
  }

  int getPriority() const {
    return getFields()->priority;
  }

  const std::string& getID() const {
    return getFields()->name;
  }

  const std::optional<MatchAction> getAclAction() const {
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

  std::optional<uint8_t> getProto() const {
    return getFields()->proto;
  }

  void setProto(const uint8_t proto) {
    writableFields()->proto = proto;
  }

  std::optional<uint8_t> getTcpFlagsBitMap() const {
    return getFields()->tcpFlagsBitMap;
  }

  void setTcpFlagsBitMap(const uint8_t flagsBitMap) {
    writableFields()->tcpFlagsBitMap = flagsBitMap;
  }

  std::optional<uint16_t> getSrcPort() const {
    return getFields()->srcPort;
  }

  void setSrcPort(const uint16_t port) {
    writableFields()->srcPort = port;
  }

  std::optional<uint16_t> getDstPort() const {
    return getFields()->dstPort;
  }

  void setDstPort(const uint16_t port) {
    writableFields()->dstPort = port;
  }

  std::optional<cfg::IpFragMatch> getIpFrag() const {
    return getFields()->ipFrag;
  }

  void setIpFrag(const cfg::IpFragMatch& frag) {
    writableFields()->ipFrag = frag;
  }

  std::optional<uint8_t> getIcmpCode() const {
    return getFields()->icmpCode;
  }

  void setIcmpCode(const uint8_t code) {
    writableFields()->icmpCode = code;
  }

  std::optional<uint8_t> getIcmpType() const {
    return getFields()->icmpType;
  }

  void setIcmpType(const uint8_t type) {
    writableFields()->icmpType = type;
  }

  std::optional<uint8_t> getDscp() const {
    return getFields()->dscp;
  }

  void setDscp(uint8_t dscp) {
    writableFields()->dscp = dscp;
  }

  std::optional<cfg::IpType> getIpType() const {
    return getFields()->ipType;
  }

  void setIpType(const cfg::IpType& ipType) {
    writableFields()->ipType = ipType;
  }

  std::optional<AclTtl> getTtl() const {
    return getFields()->ttl;
  }

  void setTtl(const AclTtl& ttl) {
    writableFields()->ttl = ttl;
  }

  std::optional<folly::MacAddress> getDstMac() const {
    return getFields()->dstMac;
  }

  void setDstMac(const folly::MacAddress& dstMac) {
    writableFields()->dstMac = dstMac;
  }

  std::optional<uint16_t> getL4SrcPort() const {
    return getFields()->l4SrcPort;
  }

  void setL4SrcPort(const uint16_t port) {
    writableFields()->l4SrcPort = port;
  }

  std::optional<uint16_t> getL4DstPort() const {
    return getFields()->l4DstPort;
  }

  void setL4DstPort(const uint16_t port) {
    writableFields()->l4DstPort = port;
  }

  std::optional<cfg::AclLookupClass> getLookupClassL2() const {
    return getFields()->lookupClassL2;
  }
  void setLookupClassL2(const cfg::AclLookupClass& lookupClassL2) {
    writableFields()->lookupClassL2 = lookupClassL2;
  }

  std::optional<cfg::AclLookupClass> getLookupClass() const {
    return getFields()->lookupClass;
  }
  void setLookupClass(const cfg::AclLookupClass& lookupClass) {
    writableFields()->lookupClass = lookupClass;
  }

  bool hasMatcher() const {
    // at least one qualifier must be specified
    return getSrcIp().first || getDstIp().first || getProto() ||
        getTcpFlagsBitMap() || getSrcPort() || getDstPort() || getIpFrag() ||
        getIcmpType() || getDscp() || getIpType() || getTtl() || getDstMac() ||
        getL4SrcPort() || getL4DstPort() || getLookupClassL2() ||
        getLookupClass();
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
