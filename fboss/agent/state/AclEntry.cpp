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
