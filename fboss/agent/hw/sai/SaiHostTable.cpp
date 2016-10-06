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

SaiHost *SaiHostTable::getSaiHost(InterfaceID intf,
                                  const folly::IPAddress &ip) const {
  VLOG(4) << "Entering " << __FUNCTION__;

  Key key(intf, ip);
  auto iter = hosts_.find(key);

  if (iter == hosts_.end()) {
    return nullptr;
  }

  return iter->second.get();
}

SaiHost* SaiHostTable::createOrUpdateSaiHost(InterfaceID intf,
                                             const folly::IPAddress &ip,
                                             const folly::MacAddress &mac) {
  VLOG(4) << "Entering " << __FUNCTION__;
  
  Key key(intf, ip);
  auto ret = hosts_.emplace(key, nullptr);
  auto &iter = ret.first;

  if (!ret.second) {
    // there was an entry already there
    return iter->second.get();
  }

  SCOPE_FAIL {
    hosts_.erase(iter);
  };

  auto newHost = folly::make_unique<SaiHost>(hw_, intf, ip, mac);
  auto hostPtr = newHost.get();

  iter->second = std::move(newHost);

  return hostPtr;
}

void SaiHostTable::removeSaiHost(InterfaceID intf,
                                 const folly::IPAddress &ip) noexcept {
  VLOG(4) << "Entering " << __FUNCTION__;

  Key key(intf, ip);
  auto iter = hosts_.find(key);

  if (iter != hosts_.end()) {
    hosts_.erase(iter);
  }
}

SaiHostTable::Key::Key(InterfaceID intf, 
                       const folly::IPAddress &ip) 
  : intf_(intf)
  , ip_(ip) {
}

bool SaiHostTable::Key::operator<(const Key &k) const{
  if (intf_ < k.intf_) {
    return true;
  }

  if (intf_ > k.intf_) {
    return false;
  }

  return (ip_ < k.ip_);
}

}} // facebook::fboss
