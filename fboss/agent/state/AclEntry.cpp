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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "folly/IPAddress.h"

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
constexpr auto kLookupClassNeighbor = "lookupClassNeighbor";
constexpr auto kLookupClassRoute = "lookupClassRoute";
constexpr auto kPacketLookupResult = "packetLookupResult";
constexpr auto kVlanID = "vlanID";
constexpr auto kAclAction = "aclAction";
constexpr auto kEnabled = "enabled";
} // namespace

namespace facebook::fboss {

state::AclTtl AclTtl::toThrift() const {
  auto aclTtl = state::AclTtl();
  aclTtl.value() = value_;
  aclTtl.mask() = mask_;
  return aclTtl;
}

AclTtl AclTtl::fromThrift(state::AclTtl const& ttl) {
  return AclTtl(ttl.get_value(), ttl.get_mask());
}

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

state::AclEntryFields AclEntryFields::toThrift() const {
  auto entry = state::AclEntryFields();
  entry.priority() = priority;
  entry.name() = name;

  if (srcIp.first) {
    entry.srcIp() = IPAddress::networkToString(srcIp);
  }
  if (dstIp.first) {
    entry.dstIp() = IPAddress::networkToString(dstIp);
  }

  entry.proto().from_optional(proto);
  entry.tcpFlagsBitMap().from_optional(tcpFlagsBitMap);
  entry.srcPort().from_optional(srcPort);
  entry.dstPort().from_optional(dstPort);
  entry.ipFrag().from_optional(ipFrag);
  entry.proto().from_optional(proto);
  entry.proto().from_optional(proto);

  if (ttl.has_value()) {
    entry.ttl() = ttl->toThrift();
  }

  entry.icmpType().from_optional(icmpType);
  entry.icmpCode().from_optional(icmpCode);
  entry.dscp().from_optional(dscp);
  entry.ipType().from_optional(ipType);

  if (dstMac.has_value()) {
    entry.dstMac() = dstMac->toString();
  }
  entry.l4SrcPort().from_optional(l4SrcPort);
  entry.l4DstPort().from_optional(l4DstPort);
  entry.lookupClassL2().from_optional(lookupClassL2);
  entry.lookupClass().from_optional(lookupClass);
  entry.lookupClassNeighbor().from_optional(lookupClassNeighbor);
  entry.lookupClassRoute().from_optional(lookupClassRoute);
  entry.packetLookupResult().from_optional(packetLookupResult);
  entry.vlanID().from_optional(vlanID);
  entry.etherType().from_optional(etherType);
  entry.actionType() = actionType;

  if (aclAction.has_value()) {
    entry.aclAction() = aclAction->toThrift();
  }
  entry.enabled().from_optional(enabled);
  return entry;
}

AclEntryFields AclEntryFields::fromThrift(state::AclEntryFields const& entry) {
  auto aclEntryFields = AclEntryFields(entry.get_priority(), entry.get_name());

  if (auto srcIp = entry.srcIp()) {
    aclEntryFields.srcIp = IPAddress::createNetwork(*srcIp);
  }
  if (auto dstIp = entry.dstIp()) {
    aclEntryFields.dstIp = IPAddress::createNetwork(*dstIp);
  }

  if (aclEntryFields.srcIp.first && aclEntryFields.dstIp.first &&
      aclEntryFields.srcIp.first.isV4() != aclEntryFields.dstIp.first.isV4()) {
    throw FbossError(
        "Unmatched ACL IP versions ",
        aclEntryFields.srcIp.first,
        " vs ",
        aclEntryFields.dstIp.first);
  }

  aclEntryFields.proto = entry.proto().to_optional();
  aclEntryFields.tcpFlagsBitMap = entry.tcpFlagsBitMap().to_optional();
  aclEntryFields.srcPort = entry.srcPort().to_optional();
  aclEntryFields.dstPort = entry.dstPort().to_optional();
  aclEntryFields.ipFrag = entry.ipFrag().to_optional();
  aclEntryFields.proto = entry.proto().to_optional();
  aclEntryFields.proto = entry.proto().to_optional();

  if (auto ttl = entry.ttl()) {
    aclEntryFields.ttl = AclTtl::fromThrift(*ttl);
  }

  aclEntryFields.icmpType = entry.icmpType().to_optional();
  aclEntryFields.icmpCode = entry.icmpCode().to_optional();
  aclEntryFields.dscp = entry.dscp().to_optional();
  aclEntryFields.ipType = entry.ipType().to_optional();

  if (auto dstMac = entry.dstMac()) {
    aclEntryFields.dstMac = folly::MacAddress(*dstMac);
  }

  aclEntryFields.l4SrcPort = entry.l4SrcPort().to_optional();
  aclEntryFields.l4DstPort = entry.l4DstPort().to_optional();
  aclEntryFields.lookupClassL2 = entry.lookupClassL2().to_optional();
  aclEntryFields.lookupClass = entry.lookupClass().to_optional();
  aclEntryFields.lookupClassNeighbor =
      entry.lookupClassNeighbor().to_optional();
  aclEntryFields.lookupClassRoute = entry.lookupClassRoute().to_optional();
  aclEntryFields.packetLookupResult = entry.packetLookupResult().to_optional();
  aclEntryFields.vlanID = entry.vlanID().to_optional();
  aclEntryFields.etherType = entry.etherType().to_optional();
  aclEntryFields.actionType = entry.get_actionType();
  if (auto aclAction = entry.aclAction()) {
    aclEntryFields.aclAction = MatchAction::fromThrift(*aclAction);
  }
  aclEntryFields.enabled = entry.enabled().to_optional();
  return aclEntryFields;
}

folly::dynamic AclEntryFields::toFollyDynamicLegacy() const {
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
  if (lookupClassNeighbor) {
    auto lookupClassNameNeighbor =
        apache::thrift::util::enumName(*lookupClassNeighbor);
    if (lookupClassNameNeighbor == nullptr) {
      throw FbossError("invalid lookupClassNeighbor");
    }
    aclEntry[kLookupClassNeighbor] = lookupClassNameNeighbor;
  }
  if (lookupClassRoute) {
    auto lookupClassNameRoute =
        apache::thrift::util::enumName(*lookupClassRoute);
    if (lookupClassNameRoute == nullptr) {
      throw FbossError("invalid lookupClassRoute");
    }
    aclEntry[kLookupClassRoute] = lookupClassNameRoute;
  }
  if (packetLookupResult) {
    aclEntry[kPacketLookupResult] =
        static_cast<uint32_t>(packetLookupResult.value());
  }
  if (vlanID) {
    aclEntry[kVlanID] = static_cast<uint32_t>(vlanID.value());
  }
  auto actionTypeName = apache::thrift::util::enumName(actionType);
  if (actionTypeName == nullptr) {
    throw FbossError("invalid actionType");
  }
  aclEntry[kActionType] = actionTypeName;
  if (aclAction) {
    aclEntry[kAclAction] = aclAction.value().toFollyDynamic();
  }
  if (enabled.has_value()) {
    aclEntry[kEnabled] = enabled.value();
  }
  aclEntry[kPriority] = priority;
  aclEntry[kName] = name;
  return aclEntry;
}

AclEntryFields AclEntryFields::fromFollyDynamicLegacy(
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
  if (aclEntryJson.find(kLookupClassNeighbor) != aclEntryJson.items().end()) {
    cfg::AclLookupClass lookupClassNeighbor;
    TEnumTraits<cfg::AclLookupClass>::findValue(
        aclEntryJson[kLookupClassNeighbor].asString().c_str(),
        &lookupClassNeighbor);
    aclEntry.lookupClassNeighbor = lookupClassNeighbor;
  }
  if (aclEntryJson.find(kLookupClassRoute) != aclEntryJson.items().end()) {
    cfg::AclLookupClass lookupClassRoute;
    TEnumTraits<cfg::AclLookupClass>::findValue(
        aclEntryJson[kLookupClassRoute].asString().c_str(), &lookupClassRoute);
    aclEntry.lookupClassRoute = lookupClassRoute;
  }
  if (aclEntryJson.find(kPacketLookupResult) != aclEntryJson.items().end()) {
    aclEntry.packetLookupResult = static_cast<cfg::PacketLookupResultType>(
        aclEntryJson[kPacketLookupResult].asInt());
  }
  if (aclEntryJson.find(kVlanID) != aclEntryJson.items().end()) {
    aclEntry.vlanID = aclEntryJson[kVlanID].asInt();
  }
  if (aclEntryJson.find(kEnabled) != aclEntryJson.items().end()) {
    aclEntry.enabled = aclEntryJson[kEnabled].asBool();
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
  // check lookupClassNeighbor is valid
  if (aclEntryJson.find(kLookupClassNeighbor) != aclEntryJson.items().end()) {
    cfg::AclLookupClass lookupClassNeighbor;
    if (!TEnumTraits<cfg::AclLookupClass>::findValue(
            aclEntryJson[kLookupClassNeighbor].asString().c_str(),
            &lookupClassNeighbor)) {
      throw FbossError(
          "Unsupported ACL LookupClassNeighbor option ",
          aclEntryJson[kLookupClassNeighbor].asString());
    }
  }
  // check lookupClassRoute is valid
  if (aclEntryJson.find(kLookupClassRoute) != aclEntryJson.items().end()) {
    cfg::AclLookupClass lookupClassRoute;
    if (!TEnumTraits<cfg::AclLookupClass>::findValue(
            aclEntryJson[kLookupClassRoute].asString().c_str(),
            &lookupClassRoute)) {
      throw FbossError(
          "Unsupported ACL LookupClassRoute option ",
          aclEntryJson[kLookupClassRoute].asString());
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

std::set<cfg::AclTableQualifier> AclEntry::getRequiredAclTableQualifiers()
    const {
  std::set<cfg::AclTableQualifier> qualifiers{};
  auto setIpQualifier = [&qualifiers](
                            auto cidr, auto v4Qualifier, auto v6Qualifier) {
    if (cidr == folly::CIDRNetwork(folly::IPAddress(), 0)) {
      return;
    }
    if (cidr.first.isV4()) {
      qualifiers.insert(v4Qualifier);
    } else {
      qualifiers.insert(v6Qualifier);
    }
  };
  setIpQualifier(
      getSrcIp(),
      cfg::AclTableQualifier::SRC_IPV4,
      cfg::AclTableQualifier::SRC_IPV6);
  setIpQualifier(
      getDstIp(),
      cfg::AclTableQualifier::DST_IPV4,
      cfg::AclTableQualifier::DST_IPV6);

  if (getProto()) {
    qualifiers.insert(cfg::AclTableQualifier::IP_PROTOCOL);
  }
  if (getTcpFlagsBitMap()) {
    qualifiers.insert(cfg::AclTableQualifier::TCP_FLAGS);
  }
  if (getSrcPort()) {
    qualifiers.insert(cfg::AclTableQualifier::SRC_PORT);
  }
  if (getDstPort()) {
    qualifiers.insert(cfg::AclTableQualifier::OUT_PORT);
  }
  if (getIpFrag()) {
    qualifiers.insert(cfg::AclTableQualifier::IP_FRAG);
  }
  if (getIcmpType() && getProto()) {
    auto proto = getProto().value();
    if (proto == 1) {
      qualifiers.insert(cfg::AclTableQualifier::ICMPV4_TYPE);
    } else {
      qualifiers.insert(cfg::AclTableQualifier::ICMPV6_TYPE);
    }
  }
  if (getDscp()) {
    qualifiers.insert(cfg::AclTableQualifier::DSCP);
  }
  if (getIpType()) {
    qualifiers.insert(cfg::AclTableQualifier::IP_TYPE);
  }
  if (getTtl()) {
    qualifiers.insert(cfg::AclTableQualifier::TTL);
  }
  if (getDstMac()) {
    qualifiers.insert(cfg::AclTableQualifier::DST_MAC);
  }
  if (getL4SrcPort()) {
    qualifiers.insert(cfg::AclTableQualifier::L4_SRC_PORT);
  }
  if (getL4DstPort()) {
    qualifiers.insert(cfg::AclTableQualifier::L4_DST_PORT);
  }
  if (getLookupClassL2()) {
    qualifiers.insert(cfg::AclTableQualifier::LOOKUP_CLASS_L2);
  }
  if (getLookupClassNeighbor()) {
    qualifiers.insert(cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR);
  }
  if (getLookupClassRoute()) {
    qualifiers.insert(cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE);
  }
  if (getPacketLookupResult()) {
    // TODO: add qualifier in AclTableQualifier enum
  }
  if (getEtherType()) {
    qualifiers.insert(cfg::AclTableQualifier::ETHER_TYPE);
  }
  return qualifiers;
}

AclEntry* AclEntry::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  AclMap* acls = (*state)->getAcls()->modify(state);
  auto newEntry = clone();
  auto* ptr = newEntry.get();
  acls->updateNode(std::move(newEntry));
  return ptr;
}

AclEntry::AclEntry(int priority, const std::string& name) {
  set<switch_state_tags::priority>(priority);
  set<switch_state_tags::name>(name);
}

AclEntry::AclEntry(int priority, std::string&& name) {
  set<switch_state_tags::priority>(priority);
  set<switch_state_tags::name>(std::move(name));
}

template class ThriftStructNode<AclEntry, state::AclEntryFields>;

} // namespace facebook::fboss
