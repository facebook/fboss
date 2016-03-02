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
  SaiHost *GetSaiHost(InterfaceID intf,
                      const folly::IPAddress &ip,
                      const folly::MacAddress &mac) const;

  /**
   * Allocates a new SaiHost if no such one exists. 
   * For the existing entry, incRefOrCreateSaiHost() will increase 
   * the reference counter by 1.
   *
   * @return The SaiHost pointer just created or found.
   */
  SaiHost *IncRefOrCreateSaiHost(InterfaceID intf,
                                 const folly::IPAddress &ip,
                                 const folly::MacAddress &mac);

  /**
   * Decrease an existing SaiHost entry's reference counter by 1.
   * Only until the reference counter is 0, the SaiHost entry
   * is deleted.
   *
   * @return nullptr, if the SaiHost entry is deleted
   * @retrun the SaiHost object that has reference counter
   *         decreased by 1, but the object is still valid as it is
   *         still referred in somewhere else
   */
  SaiHost *DerefSaiHost(InterfaceID intf,
                        const folly::IPAddress &ip,
                        const folly::MacAddress &mac) noexcept;

private:
  struct Key {
    Key(InterfaceID intf, 
        const folly::IPAddress &ip,
        const folly::MacAddress &mac);
    bool operator<(const Key &k) const;

    InterfaceID intf_;
    folly::IPAddress ip_;
    folly::MacAddress mac_;
  };

  typedef std::pair<std::unique_ptr<SaiHost>, uint32_t> HostMapNode;
  typedef boost::container::flat_map<Key, HostMapNode> HostMap;

  const SaiSwitch *hw_;
  HostMap hosts_;
};

}} // facebook::fboss
