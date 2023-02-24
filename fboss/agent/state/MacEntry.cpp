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
constexpr auto kMacEntryPort = "portId";
constexpr auto kClassID = "classID";
constexpr auto kType = "type";
} // namespace

namespace facebook::fboss {

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

template class ThriftStructNode<MacEntry, state::MacEntryFields>;

} // namespace facebook::fboss
