/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/MacEntry.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/StateUtils.h"

#include <sstream>

namespace {
constexpr auto kMac = "mac";
constexpr auto kPort = "portId";
constexpr auto kClassID = "classID";
constexpr auto kType = "type";
} // namespace

namespace facebook::fboss {

folly::dynamic MacEntryFields::toFollyDynamic() const {
  folly::dynamic macEntry = folly::dynamic::object;
  macEntry[kMac] = mac_.toString();
  macEntry[kPort] = portDescr_.toFollyDynamic();
  if (classID_.has_value()) {
    macEntry[kClassID] = static_cast<int>(classID_.value());
  }
  macEntry[kType] = static_cast<int>(type_);

  return macEntry;
}

MacEntryFields MacEntryFields::fromFollyDynamic(const folly::dynamic& jsonStr) {
  folly::MacAddress mac(jsonStr[kMac].stringPiece());
  auto portDescr = PortDescriptor::fromFollyDynamic(jsonStr[kPort]);

  MacEntryType type{MacEntryType::DYNAMIC_ENTRY};
  if (jsonStr.find(kType) != jsonStr.items().end()) {
    type = static_cast<MacEntryType>(jsonStr[kType].asInt());
  }
  if (jsonStr.find(kClassID) != jsonStr.items().end()) {
    auto classID = cfg::AclLookupClass(jsonStr[kClassID].asInt());
    return MacEntryFields(mac, portDescr, classID, type);
  } else {
    return MacEntryFields(mac, portDescr, std::nullopt, type);
  }
}

std::string MacEntry::str() const {
  std::ostringstream os;

  auto classIDStr = getClassID().has_value()
      ? folly::to<std::string>(static_cast<int>(getClassID().value()))
      : "None";

  os << "MacEntry:: MAC: " << getMac().toString() << " " << getPort().str()
     << " classID: " << classIDStr << " "
     << " type: "
     << (getType() == MacEntryType::STATIC_ENTRY ? "static" : "dynamic");

  return os.str();
}

template class NodeBaseT<MacEntry, MacEntryFields>;

} // namespace facebook::fboss
