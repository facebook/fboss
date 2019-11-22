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
} // namespace

namespace facebook {
namespace fboss {

folly::dynamic MacEntryFields::toFollyDynamic() const {
  folly::dynamic macEntry = folly::dynamic::object;
  macEntry[kMac] = mac_.toString();
  macEntry[kPort] = portDescr_.toFollyDynamic();

  return macEntry;
}

MacEntryFields MacEntryFields::fromFollyDynamic(const folly::dynamic& jsonStr) {
  folly::MacAddress mac(jsonStr[kMac].stringPiece());
  auto portDescr = PortDescriptor::fromFollyDynamic(jsonStr[kPort]);

  return MacEntryFields(mac, portDescr);
}

std::string MacEntry::str() const {
  std::ostringstream os;

  os << "MacEntry:: MAC: " << getMac().toString() << " "
     << getPortDescriptor().str();

  return os.str();
}

template class NodeBaseT<MacEntry, MacEntryFields>;

} // namespace fboss
} // namespace facebook
