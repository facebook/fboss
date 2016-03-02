/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#include "SaiHostTable.h"
#include "SaiHost.h"
#include "SaiSwitch.h"

namespace facebook { namespace fboss {

SaiHostTable::SaiHostTable(const SaiSwitch *hw) : hw_(hw) {
  VLOG(4) << "Entering " << __FUNCTION__;
}

SaiHostTable::~SaiHostTable() {
  VLOG(4) << "Entering " << __FUNCTION__;
}

SaiHost* SaiHostTable::IncRefOrCreateSaiHost(InterfaceID intf,
                                             const folly::IPAddress &ip,
                                             const folly::MacAddress &mac) {
  VLOG(4) << "Entering " << __FUNCTION__;
  
  Key key(intf, ip, mac);
  auto ret = hosts_.emplace(key, std::make_pair(nullptr, 1));
  auto &iter = ret.first;

  if (!ret.second) {
    // there was an entry already there
    iter->second.second++;  // increase the reference counter
    return iter->second.first.get();
  }

  SCOPE_FAIL {
    hosts_.erase(iter);
  };

  auto newHost = folly::make_unique<SaiHost>(hw_, intf, ip, mac);
  auto hostPtr = newHost.get();

  iter->second.first = std::move(newHost);

  return hostPtr;
}

SaiHost *SaiHostTable::GetSaiHost(InterfaceID intf,
                                  const folly::IPAddress &ip,
                                  const folly::MacAddress &mac) const {
  VLOG(4) << "Entering " << __FUNCTION__;

  Key key(intf, ip, mac);
  auto iter = hosts_.find(key);

  if (iter == hosts_.end()) {
    return nullptr;
  }

  return iter->second.first.get();
}

SaiHost *SaiHostTable::DerefSaiHost(InterfaceID intf,
                                    const folly::IPAddress &ip,
                                    const folly::MacAddress &mac) noexcept {
  VLOG(4) << "Entering " << __FUNCTION__;

  Key key(intf, ip, mac);
  auto iter = hosts_.find(key);

  if (iter == hosts_.end()) {
    return nullptr;
  }

  auto &entry = iter->second;
  CHECK_GT(entry.second, 0);

  if (--entry.second == 0) {
    hosts_.erase(iter);
    return nullptr;
  }

  return entry.first.get();
}

SaiHostTable::Key::Key(InterfaceID intf, 
                       const folly::IPAddress &ip,
                       const folly::MacAddress &mac) 
  : intf_(intf)
  , ip_(ip)
  , mac_(mac) {
}

bool SaiHostTable::Key::operator<(const Key &k) const{
  if (intf_ < k.intf_) {
    return true;
  }

  if (intf_ > k.intf_) {
    return false;
  }

  if (ip_ < k.ip_) {
    return true;
  }

  if (ip_ > k.ip_) {
    return false;
  }

  return (mac_ < k.mac_);
}

}} // facebook::fboss
