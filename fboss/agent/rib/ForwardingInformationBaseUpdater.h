/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook {
namespace fboss {
class SwitchState;
} // namespace fboss
} // namespace facebook

namespace facebook {
namespace fboss {
namespace rib {

class ForwardingInformationBaseUpdater {
 public:
  ForwardingInformationBaseUpdater(
      RouterID vrf,
      const IPv4NetworkToRouteMap& v4NetworkToRoute,
      const IPv6NetworkToRouteMap& v6NetworkToRoute);

  std::shared_ptr<SwitchState> operator()(
      const std::shared_ptr<SwitchState>& state);

 private:
  template <typename AddressT>
  std::unique_ptr<typename facebook::fboss::ForwardingInformationBase<AddressT>>
  createUpdatedFib(
      const facebook::fboss::rib::NetworkToRouteMap<AddressT>& ribRange);

  RouterID vrf_;
  const IPv4NetworkToRouteMap& v4NetworkToRoute_;
  const IPv6NetworkToRouteMap& v6NetworkToRoute_;
};

} // namespace rib
} // namespace fboss
} // namespace facebook
