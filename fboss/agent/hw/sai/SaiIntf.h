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

#include <set>

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/types.h"

extern "C" {
#include "saitypes.h"
#include "sairouterintf.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;
class SaiStation;
class SaiHost;

class SaiIntf {
public:
  explicit SaiIntf(const SaiSwitch *hw);
  virtual ~SaiIntf();

  sai_object_id_t GetIfId() const {
    return saiIfId_;
  }

  const std::shared_ptr<Interface> &GetInterface() const {
    return intf_;
  }
  void Program(const std::shared_ptr<Interface> &intf);

private:
  // no copy or assignment
  SaiIntf(SaiIntf const &) = delete;
  SaiIntf &operator=(SaiIntf const &) = delete;

  void removeHost(const folly::IPAddress& addr);

  enum {
    SAI_ROUTER_IF_ATTR_COUNT = 7
  };

  const SaiSwitch *hw_;
  std::shared_ptr<Interface> intf_ {0};
  sai_object_id_t saiIfId_ {SAI_NULL_OBJECT_ID};
  // The interface addresses that have SaiHost object created
  std::set<folly::IPAddress> hosts_;

  sai_router_interface_api_t *saiRouterInterfaceApi_ { nullptr };
  

  // TODO: we now generate one station entry per interface, even if all
  //       interfaces are sharing the same MAC. We can save some station
  //       entries by just generate one per different MAC. But with
  //       the small number of interfaces we have, no gain for that now.
  std::unique_ptr<SaiStation> station_;

};

}} // facebook::fboss
