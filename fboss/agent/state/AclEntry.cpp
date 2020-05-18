/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AclEntry.h"
#include <folly/Conv.h>
#include <folly/MacAddress.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/StateUtils.h"

using apache::thrift::TEnumTraits;
using folly::IPAddress;

namespace {
constexpr auto kPriority = "priority";
constexpr auto kName = "name";
constexpr auto kActionType = "actionType";
constexpr auto kSrcIp = "srcIp";
constexpr auto kDstIp = "dstIp";
constexpr auto kL4SrcPort = "l4SrcPort";
constexpr auto kL4DstPort = "l4DstPort";
constexpr auto kProto = "proto";
constexpr auto kTcpFlagsBitMap = "tcpFlagsBitMap";
constexpr auto kSrcPort = "srcPort";
constexpr auto kDstPort = "dstPort";
constexpr auto kIpFrag = "ipFrag";
constexpr auto kIcmpCode = "icmpCode";
constexpr auto kIcmpType = "icmpType";
constexpr auto kDscp = "dscp";
constexpr auto kPortName = "portName";
constexpr auto kDstMac = "dstMac";
constexpr auto kIpType = "IpType";
constexpr auto kTtl = "ttl";
constexpr auto kTtlValue = "value";
constexpr auto kTtlMask = "mask";
constexpr auto kLookupClassL2 = "lookupClassL2";
constexpr auto kLookupClass = "lookupClass";
constexpr auto kAclAction = "aclAction";
} // namespace

namespace facebook::fboss {

folly::dynamic AclTtl::toFollyDynamic() const {
  folly::dynamic ttl = folly::dynamic::object;
  ttl[kTtlValue] = static_cast<uint16_t>(value_);
  ttl[kTtlMask] = static_cast<uint16_t>(mask_);
  return ttl;
}

AclTtl AclTtl::fromFollyDynamic(const folly::dynamic& ttlJson) {
  if (ttlJson.find(kTtlValue) == ttlJson.items().end()) {
    throw FbossError("ttl should have a value set");
  }
  if (ttlJson.find(kTtlMask) == ttlJson.items().end()) {
    throw FbossError("ttl should have a mask set");
  }
  return AclTtl(ttlJson[kTtlValue].asInt(), ttlJson[kTtlMask].asInt());
}

folly::dynamic AclEntryFields::toFollyDynamic() const {
  folly::dynamic aclEntry = folly::dynamic::object;
  if (srcIp.first) {
    aclEntry[kSrcIp] = IPAddress::networkToString(srcIp);
  }
  if (dstIp.first) {
    aclEntry[kDstIp] = IPAddress::networkToString(dstIp);
  }
  if (dstMac) {
    aclEntry[kDstMac] = dstMac->toString();
  }
  if (proto) {
    aclEntry[kProto] = static_cast<uint8_t>(proto.value());
  }
  if (tcpFlagsBitMap) {
    aclEntry[kTcpFlagsBitMap] = static_cast<uint8_t>(tcpFlagsBitMap.value());
  }
  if (srcPort) {
    aclEntry[kSrcPort] = static_cast<uint16_t>(srcPort.value());
  }
  if (dstPort) {
    aclEntry[kDstPort] = static_cast<uint16_t>(dstPort.value());
  }
  if (ipFrag) {
    auto ipFragName = apache::thrift::util::enumName(*ipFrag);
    if (ipFragName == nullptr) {
      throw FbossError("invalid ipFrag");
    }
    aclEntry[kIpFrag] = ipFragName;
  }
  if (icmpCode) {
    aclEntry[kIcmpCode] = static_cast<uint8_t>(icmpCode.value());
  }
  if (icmpType) {
    aclEntry[kIcmpType] = static_cast<uint8_t>(icmpType.value());
  }
  if (dscp) {
    aclEntry[kDscp] = static_cast<uint8_t>(dscp.value());
  }
  if (ipType) {
    aclEntry[kIpType] = static_cast<uint16_t>(ipType.value());
  }
  if (ttl) {
    aclEntry[kTtl] = ttl.value().toFollyDynamic();
  }
  if (l4SrcPort) {
    aclEntry[kL4SrcPort] = static_cast<uint16_t>(l4SrcPort.value());
  }
  if (l4DstPort) {
    aclEntry[kL4DstPort] = static_cast<uint16_t>(l4DstPort.value());
  }
  if (lookupClassL2) {
    auto lookupClassNameL2 = apache::thrift::util::enumName(*lookupClassL2);
    if (lookupClassNameL2 == nullptr) {
      throw FbossError("invalid lookupClassL2");
    }
    aclEntry[kLookupClassL2] = lookupClassNameL2;
  }
  if (lookupClass) {
    auto lookupClassName = apache::thrift::util::enumName(*lookupClass);
    if (lookupClassName == nullptr) {
      throw FbossError("invalid lookupClass");
    }
    aclEntry[kLookupClass] = lookupClassName;
  }
  auto actionTypeName = apache::thrift::util::enumName(actionType);
  if (actionTypeName == nullptr) {
    throw FbossError("invalid actionType");
  }
  aclEntry[kActionType] = actionTypeName;
  if (aclAction) {
    aclEntry[kAclAction] = aclAction.value().toFollyDynamic();
  }
  aclEntry[kPriority] = priority;
  aclEntry[kName] = name;
  return aclEntry;
}

AclEntryFields AclEntryFields::fromFollyDynamic(
    const folly::dynamic& aclEntryJson) {
  AclEntryFields aclEntry(
      aclEntryJson[kPriority].asInt(), aclEntryJson[kName].asString());
  if (aclEntryJson.find(kSrcIp) != aclEntryJson.items().end()) {
    aclEntry.srcIp = IPAddress::createNetwork(aclEntryJson[kSrcIp].asString());
  }
  if (aclEntryJson.find(kDstIp) != aclEntryJson.items().end()) {
    aclEntry.dstIp = IPAddress::createNetwork(aclEntryJson[kDstIp].asString());
  }
  if (aclEntry.srcIp.first && aclEntry.dstIp.first &&
      aclEntry.srcIp.first.isV4() != aclEntry.dstIp.first.isV4()) {
    throw FbossError(
        "Unmatched ACL IP versions ",
        aclEntryJson[kSrcIp].asString(),
        " vs ",
        aclEntryJson[kDstIp].asString());
  }
  if (aclEntryJson.find(kDstMac) != aclEntryJson.items().end()) {
    aclEntry.dstMac = folly::MacAddress(aclEntryJson[kDstMac].asString());
  }
  if (aclEntryJson.find(kProto) != aclEntryJson.items().end()) {
    aclEntry.proto = aclEntryJson[kProto].asInt();
  }
  if (aclEntryJson.find(kTcpFlagsBitMap) != aclEntryJson.items().end()) {
    aclEntry.tcpFlagsBitMap = aclEntryJson[kTcpFlagsBitMap].asInt();
  }
  if (aclEntryJson.find(kSrcPort) != aclEntryJson.items().end()) {
    aclEntry.srcPort = aclEntryJson[kSrcPort].asInt();
  }
  if (aclEntryJson.find(kDstPort) != aclEntryJson.items().end()) {
    aclEntry.dstPort = aclEntryJson[kDstPort].asInt();
  }
  if (aclEntryJson.find(kL4SrcPort) != aclEntryJson.items().end()) {
    aclEntry.l4SrcPort = aclEntryJson[kL4SrcPort].asInt();
  }
  if (aclEntryJson.find(kL4DstPort) != aclEntryJson.items().end()) {
    aclEntry.l4DstPort = aclEntryJson[kL4DstPort].asInt();
  }
  if (aclEntryJson.find(kIpFrag) != aclEntryJson.items().end()) {
    cfg::IpFragMatch ipFrag;
    TEnumTraits<cfg::IpFragMatch>::findValue(
        aclEntryJson[kIpFrag].asString().c_str(), &ipFrag);
    aclEntry.ipFrag = ipFrag;
  }
  if (aclEntryJson.find(kIcmpType) != aclEntryJson.items().end()) {
    aclEntry.icmpType = aclEntryJson[kIcmpType].asInt();
  }
  if (aclEntryJson.find(kIcmpCode) != aclEntryJson.items().end()) {
    aclEntry.icmpCode = aclEntryJson[kIcmpCode].asInt();
  }
  if (aclEntryJson.find(kDscp) != aclEntryJson.items().end()) {
    aclEntry.dscp = aclEntryJson[kDscp].asInt();
  }
  if (aclEntryJson.find(kIpType) != aclEntryJson.items().end()) {
    aclEntry.ipType = static_cast<cfg::IpType>(aclEntryJson[kIpType].asInt());
  }
  if (aclEntryJson.find(kTtl) != aclEntryJson.items().end()) {
    aclEntry.ttl = AclTtl::fromFollyDynamic(aclEntryJson[kTtl]);
  }
  if (aclEntryJson.find(kLookupClassL2) != aclEntryJson.items().end()) {
    cfg::AclLookupClass lookupClassL2;
    TEnumTraits<cfg::AclLookupClass>::findValue(
        aclEntryJson[kLookupClassL2].asString().c_str(), &lookupClassL2);
    aclEntry.lookupClassL2 = lookupClassL2;
  }
  if (aclEntryJson.find(kLookupClass) != aclEntryJson.items().end()) {
    cfg::AclLookupClass lookupClass;
    TEnumTraits<cfg::AclLookupClass>::findValue(
        aclEntryJson[kLookupClass].asString().c_str(), &lookupClass);
    aclEntry.lookupClass = lookupClass;
  }
  TEnumTraits<cfg::AclActionType>::findValue(
      aclEntryJson[kActionType].asString().c_str(), &aclEntry.actionType);
  if (aclEntryJson.find(kAclAction) != aclEntryJson.items().end()) {
    aclEntry.aclAction =
        MatchAction::fromFollyDynamic(aclEntryJson[kAclAction]);
  }

  return aclEntry;
}

void AclEntryFields::checkFollyDynamic(const folly::dynamic& aclEntryJson) {
  // check src ip and dst ip are of the same type
  if (aclEntryJson.find(kSrcIp) != aclEntryJson.items().end() &&
      aclEntryJson.find(kDstIp) != aclEntryJson.items().end()) {
    auto src = IPAddress::createNetwork(aclEntryJson[kSrcIp].asString());
    auto dst = IPAddress::createNetwork(aclEntryJson[kDstIp].asString());
    if (src.first.isV4() != dst.first.isV4()) {
      throw FbossError(
          "Unmatched ACL IP versions ",
          aclEntryJson[kSrcIp].asString(),
          " vs ",
          aclEntryJson[kDstIp].asString(),
          "; source and destination IPs must be of the same type");
    }
  }
  // check lookupClass is valid
  if (aclEntryJson.find(kLookupClass) != aclEntryJson.items().end()) {
    cfg::AclLookupClass lookupClass;
    if (!TEnumTraits<cfg::AclLookupClass>::findValue(
            aclEntryJson[kLookupClass].asString().c_str(), &lookupClass)) {
      throw FbossError(
          "Unsupported ACL Lookup Class option ",
          aclEntryJson[kLookupClass].asString());
    }
  }
  // check lookupClassL2 is valid
  if (aclEntryJson.find(kLookupClassL2) != aclEntryJson.items().end()) {
    cfg::AclLookupClass lookupClassL2;
    if (!TEnumTraits<cfg::AclLookupClass>::findValue(
            aclEntryJson[kLookupClassL2].asString().c_str(), &lookupClassL2)) {
      throw FbossError(
          "Unsupported ACL Lookup ClassL2 option ",
          aclEntryJson[kLookupClassL2].asString());
    }
  }
  // check ipFrag is valid
  if (aclEntryJson.find(kIpFrag) != aclEntryJson.items().end()) {
    cfg::IpFragMatch ipFrag;
    if (!TEnumTraits<cfg::IpFragMatch>::findValue(
            aclEntryJson[kIpFrag].asString().c_str(), &ipFrag)) {
      throw FbossError(
          "Unsupported ACL IP fragmentation option ",
          aclEntryJson[kIpFrag].asString());
    }
  }
  // check action is valid
  cfg::AclActionType aclActionType;
  if (!TEnumTraits<cfg::AclActionType>::findValue(
          aclEntryJson[kActionType].asString().c_str(), &aclActionType)) {
    throw FbossError(
        "Unsupported ACL action ", aclEntryJson[kActionType].asString());
  }
  // check icmp type exists when icmp code exist
  if (aclEntryJson.find(kIcmpCode) != aclEntryJson.items().end() &&
      aclEntryJson.find(kIcmpType) == aclEntryJson.items().end()) {
    throw FbossError("icmp type must be set when icmp code is set");
  }
  // the value of icmp type must be 0~255
  if (aclEntryJson.find(kIcmpType) != aclEntryJson.items().end() &&
      (aclEntryJson[kIcmpType].asInt() < 0 ||
       aclEntryJson[kIcmpType].asInt() > kMaxIcmpType)) {
    throw FbossError(
        "icmp type value must be between 0 and ", std::to_string(kMaxIcmpType));
  }
  // the value of icmp code must be 0~255
  if (aclEntryJson.find(kIcmpCode) != aclEntryJson.items().end() &&
      (aclEntryJson[kIcmpCode].asInt() < 0 ||
       aclEntryJson[kIcmpCode].asInt() > kMaxIcmpCode)) {
    throw FbossError(
        "icmp code value must be between 0 and ", std::to_string(kMaxIcmpCode));
  }
  // check the "proto" is either "icmp" or "icmpv6" when icmptype is set
  if (aclEntryJson.find(kIcmpType) != aclEntryJson.items().end() &&
      (aclEntryJson.find(kProto) == aclEntryJson.items().end() ||
       !(aclEntryJson[kProto] == kProtoIcmp ||
         aclEntryJson[kProto] == kProtoIcmpv6))) {
    throw FbossError(
        "proto must be either icmp or icmpv6 ", "if icmp type is set");
  }

  if (aclEntryJson.find(kL4SrcPort) != aclEntryJson.items().end() &&
      (aclEntryJson[kL4SrcPort].asInt() < 0 ||
       aclEntryJson[kL4SrcPort].asInt() > kMaxL4Port)) {
    throw FbossError(
        "L4 source port must be between 0 and ", std::to_string(kMaxL4Port));
  }

  if (aclEntryJson.find(kL4DstPort) != aclEntryJson.items().end() &&
      (aclEntryJson[kL4DstPort].asInt() < 0 ||
       aclEntryJson[kL4DstPort].asInt() > kMaxL4Port)) {
    throw FbossError(
        "L4 destination port must be between 0 and ",
        std::to_string(kMaxL4Port));
  }
}

AclEntry::AclEntry(int priority, const std::string& name)
    : NodeBaseT(priority, name) {}

template class NodeBaseT<AclEntry, AclEntryFields>;

} // namespace facebook::fboss
