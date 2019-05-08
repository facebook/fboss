// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/bcm/BcmNextHop.h"

namespace facebook {
namespace fboss {

template <typename NextHopKeyT, typename NextHopT>
const NextHopT* BcmNextHopTable<NextHopKeyT, NextHopT>::getNextHopIf(
    const NextHopKeyT& key) const {
  return nexthops_.get(key);
}

template <typename NextHopKeyT, typename NextHopT>
const NextHopT* BcmNextHopTable<NextHopKeyT, NextHopT>::getNextHop(
    const NextHopKeyT& key) const {
  auto rv = getNextHopIf(key);
  if (!rv) {
    throw FbossError("nexthop ", key.str(), " not found");
  }
  return rv;
}

template <typename NextHopKeyT, typename NextHopT>
std::shared_ptr<NextHopT>
BcmNextHopTable<NextHopKeyT, NextHopT>::referenceOrEmplaceNextHop(
    const NextHopKeyT& key) {
  auto rv = nexthops_.refOrEmplace(key, hw_, key);
  if (rv.second) {
    XLOG(DBG3) << "inserted reference to next hop " << key.str();
  } else {
    XLOG(DBG3) << "accessed reference to next hop " << key.str();
  }
  return rv.first;
}
} // namespace fboss
} // namespace facebook
