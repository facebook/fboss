/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmRtag7LoadBalancer.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmRtag7Module.h"
#include "fboss/agent/state/LoadBalancer.h"

#include <memory>
#include <utility>

namespace facebook::fboss {

BcmRtag7LoadBalancer::BcmRtag7LoadBalancer(const BcmSwitch* hw)
    : rtag7Modules_(), hw_(hw) {}

BcmRtag7LoadBalancer::~BcmRtag7LoadBalancer() {}

void BcmRtag7LoadBalancer::addLoadBalancer(
    const std::shared_ptr<LoadBalancer>& loadBalancer) {
  BcmRtag7Module::ModuleControl moduleControl;
  BcmRtag7Module::OutputSelectionControl outputSelectionControl;

  // The assignment of module A to ECMP and module B to LAG is arbitrary with
  // the exception that it maintains backwards-compatibility.
  switch (loadBalancer->getID()) {
    case LoadBalancerID::ECMP:
      moduleControl = BcmRtag7Module::kModuleAControl();
      outputSelectionControl = BcmRtag7Module::kEcmpOutputSelectionControl();
      break;
    case LoadBalancerID::AGGREGATE_PORT:
      moduleControl = BcmRtag7Module::kModuleBControl();
      outputSelectionControl = BcmRtag7Module::kTrunkOutputSelectionControl();
      break;
  }

  auto module = std::make_unique<BcmRtag7Module>(
      moduleControl, outputSelectionControl, hw_);
  module->init(loadBalancer);

  bool inserted;
  std::tie(std::ignore, inserted) =
      rtag7Modules_.emplace(loadBalancer->getID(), std::move(module));

  if (!inserted) {
    throw FbossError(
        "failed to add LoadBalancer ",
        loadBalancer->getID(),
        ": corresponding LoadBalancer already exists");
  }
}

void BcmRtag7LoadBalancer::deleteLoadBalancer(
    const std::shared_ptr<LoadBalancer>& loadBalancer) {
  auto numErased = rtag7Modules_.erase(loadBalancer->getID());

  if (numErased == 0) {
    throw FbossError(
        "No LoadBalancer with id ", loadBalancer->getID(), " found");
  } else if (numErased > 1) {
    throw FbossError(
        "Multiple LoadBalancers with id ", loadBalancer->getID(), " found");
  }
}

void BcmRtag7LoadBalancer::programLoadBalancer(
    const std::shared_ptr<LoadBalancer>& oldLoadBalancer,
    const std::shared_ptr<LoadBalancer>& newLoadBalancer) {
  CHECK_EQ(oldLoadBalancer->getID(), newLoadBalancer->getID());

  auto it = rtag7Modules_.find(newLoadBalancer->getID());
  if (it == rtag7Modules_.end()) {
    throw FbossError(
        "failed to program LoadBalancer ",
        newLoadBalancer->getID(),
        ": no corresponding LoadBalancer");
  }

  it->second->program(oldLoadBalancer, newLoadBalancer);
}

} // namespace facebook::fboss
