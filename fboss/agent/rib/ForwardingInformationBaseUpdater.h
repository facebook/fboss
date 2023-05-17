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
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

class SwitchState;
class SwitchIdScopeResolver;

class ForwardingInformationBaseUpdater {
 public:
  ForwardingInformationBaseUpdater(
      const SwitchIdScopeResolver* resolver,
      RouterID vrf,
      const IPv4NetworkToRouteMap& v4NetworkToRoute,
      const IPv6NetworkToRouteMap& v6NetworkToRoute,
      const LabelToRouteMap& labelToRoute);

  std::shared_ptr<SwitchState> operator()(
      const std::shared_ptr<SwitchState>& state);

 private:
  /*
   * Return updated FIB on change, null otherwise
   */
  template <typename AddressT>
  std::shared_ptr<typename facebook::fboss::ForwardingInformationBase<AddressT>>
  createUpdatedFib(
      const facebook::fboss::NetworkToRouteMap<AddressT>& rib,
      const std::shared_ptr<
          facebook::fboss::ForwardingInformationBase<AddressT>>& fib);
  std::shared_ptr<facebook::fboss::LabelForwardingInformationBase>
  createUpdatedLabelFib(
      const facebook::fboss::NetworkToRouteMap<LabelID>& rib,
      std::shared_ptr<facebook::fboss::LabelForwardingInformationBase> fib);

  const SwitchIdScopeResolver* resolver_;
  RouterID vrf_;
  const IPv4NetworkToRouteMap& v4NetworkToRoute_;
  const IPv6NetworkToRouteMap& v6NetworkToRoute_;
  const LabelToRouteMap& labelToRoute_;
};

} // namespace facebook::fboss
