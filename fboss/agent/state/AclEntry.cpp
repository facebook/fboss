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

#include "fboss/agent/state/NodeBase-defs.h"

using folly::IPAddress;

namespace {
constexpr auto kId = "id";
constexpr auto kAction = "action";
constexpr auto kSrcIp = "srcIp";
constexpr auto kDstIp = "dstIp";
constexpr auto kL4SrcPort = "l4SrcPort";
constexpr auto kL4DstPort = "l4DstPort";
constexpr auto kProto = "proto";
constexpr auto kTcpFlags = "tcpFlags";
constexpr auto kTcpFlagsMask = "tcpFlagsMask";
constexpr auto kSrcPort = "srcPort";
constexpr auto kDstPort = "dstPort";
}

namespace facebook { namespace fboss {

folly::dynamic AclEntryFields::toFollyDynamic() const {
  folly::dynamic aclEntry = folly::dynamic::object;
  if (srcIp.first) {
    aclEntry[kSrcIp] = IPAddress::networkToString(srcIp);
  }
  if (dstIp.first) {
    aclEntry[kDstIp] = IPAddress::networkToString(dstIp);
  }
  if (l4SrcPort) {
    aclEntry[kL4SrcPort] = static_cast<uint16_t>(l4SrcPort);
  }
  if (l4DstPort) {
    aclEntry[kL4DstPort] = static_cast<uint16_t>(l4DstPort);
  }
  if (proto) {
    aclEntry[kProto] = static_cast<uint16_t>(proto);
  }
  if (tcpFlags) {
    aclEntry[kTcpFlags] = static_cast<uint16_t>(tcpFlags);
  }
  if (tcpFlagsMask) {
    aclEntry[kTcpFlagsMask] = static_cast<uint16_t>(tcpFlagsMask);
  }
  if (srcPort) {
    aclEntry[kSrcPort] = static_cast<uint16_t>(srcPort);
  }
  if (dstPort) {
    aclEntry[kDstPort] = static_cast<uint16_t>(dstPort);
  }
  auto itr_action = cfg::_AclAction_VALUES_TO_NAMES.find(action);
  CHECK(itr_action != cfg::_AclAction_VALUES_TO_NAMES.end());
  aclEntry[kAction] = itr_action->second;
  aclEntry[kId] = static_cast<uint32_t>(id);
  return aclEntry;
}

AclEntryFields AclEntryFields::fromFollyDynamic(
    const folly::dynamic& aclEntryJson) {
  AclEntryFields aclEntry(AclEntryID(aclEntryJson[kId].asInt()));
  if (aclEntryJson.find(kSrcIp) != aclEntryJson.items().end()) {
    aclEntry.srcIp = IPAddress::createNetwork(
      aclEntryJson[kSrcIp].asString());
  }
  if (aclEntryJson.find(kDstIp) != aclEntryJson.items().end()) {
    aclEntry.dstIp = IPAddress::createNetwork(
      aclEntryJson[kDstIp].asString());
  }
  if (aclEntry.srcIp.first && aclEntry.dstIp.first &&
      aclEntry.srcIp.first.isV4() != aclEntry.dstIp.first.isV4()) {
    throw FbossError(
      "Unmatched ACL IP versions ",
      aclEntryJson[kSrcIp].asString(),
      " vs ",
      aclEntryJson[kDstIp].asString()
    );
  }
  if (aclEntryJson.find(kL4SrcPort) != aclEntryJson.items().end()) {
    aclEntry.l4SrcPort = aclEntryJson[kL4SrcPort].asInt();
  }
  if (aclEntryJson.find(kL4DstPort) != aclEntryJson.items().end()) {
    aclEntry.l4DstPort = aclEntryJson[kL4DstPort].asInt();
  }
  if (aclEntryJson.find(kProto) != aclEntryJson.items().end()) {
    aclEntry.proto = aclEntryJson[kProto].asInt();
  }
  if (aclEntryJson.find(kTcpFlags) != aclEntryJson.items().end()) {
    aclEntry.tcpFlags = aclEntryJson[kTcpFlags].asInt();
  }
  if (aclEntryJson.find(kTcpFlagsMask) != aclEntryJson.items().end()) {
    aclEntry.tcpFlagsMask = aclEntryJson[kTcpFlagsMask].asInt();
  }
  if (aclEntryJson.find(kSrcPort) != aclEntryJson.items().end()) {
    aclEntry.srcPort = aclEntryJson[kSrcPort].asInt();
  }
  if (aclEntryJson.find(kDstPort) != aclEntryJson.items().end()) {
    aclEntry.dstPort = aclEntryJson[kDstPort].asInt();
  }
  auto itr_action = cfg::_AclAction_NAMES_TO_VALUES.find(
    aclEntryJson[kAction].asString().c_str());
  if (itr_action == cfg::_AclAction_NAMES_TO_VALUES.end()) {
    throw FbossError(
      "Unsupported ACL action ", aclEntryJson[kAction].asString());
  }
  aclEntry.action = cfg::AclAction(itr_action->second);
  return aclEntry;
}

AclEntry::AclEntry(AclEntryID id)
  : NodeBaseT(id) {
}

template class NodeBaseT<AclEntry, AclEntryFields>;

}} // facebook::fboss
