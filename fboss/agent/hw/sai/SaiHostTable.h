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
#pragma once

#include <boost/container/flat_map.hpp>
#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

#include "fboss/agent/types.h"

namespace facebook { namespace fboss {

class SaiSwitch;
class SaiHost;

class SaiHostTable {
public:
  explicit SaiHostTable(const SaiSwitch *hw);
  virtual ~SaiHostTable();

  // return nullptr if not found
  SaiHost *getSaiHost(InterfaceID intf, const folly::IPAddress &ip) const;
  /**
   * Allocates a new SaiHost if no such one exists or  
   * updates an existing one. 
   *
   * @return The SaiHost pointer just created or found.
   */
  SaiHost *createOrUpdateSaiHost(InterfaceID intf,
                                 const folly::IPAddress &ip,
                                 const folly::MacAddress &mac);

  /**
   * Remove an existing SaiHost entry
   */
   void removeSaiHost(InterfaceID intf,
                      const folly::IPAddress &ip) noexcept;

private:
  struct Key {
    Key(InterfaceID intf, 
        const folly::IPAddress &ip);
    bool operator<(const Key &k) const;

    InterfaceID intf_;
    folly::IPAddress ip_;
  };

  typedef boost::container::flat_map<Key, std::unique_ptr<SaiHost>> HostMap;

  const SaiSwitch *hw_;
  HostMap hosts_;
};

}} // facebook::fboss
