/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

namespace {
constexpr auto kFibV4{"fibV4"};
constexpr auto kFibV6{"fibV6"};
constexpr auto kVrf{"vrf"};
} // namespace

namespace facebook::fboss {

ForwardingInformationBaseContainerFields::
    ForwardingInformationBaseContainerFields(RouterID vrf)
    : vrf(vrf) {
  fibV4 = std::make_shared<ForwardingInformationBaseV4>();
  fibV6 = std::make_shared<ForwardingInformationBaseV6>();
}

folly::dynamic ForwardingInformationBaseContainerFields::toFollyDynamicLegacy()
    const {
  folly::dynamic json = folly::dynamic::object;
  json[kVrf] = static_cast<int>(vrf);
  json[kFibV4] = fibV4->toFollyDynamicLegacy();
  json[kFibV6] = fibV6->toFollyDynamicLegacy();
  return json;
}

ForwardingInformationBaseContainerFields
ForwardingInformationBaseContainerFields::fromFollyDynamicLegacy(
    const folly::dynamic& dyn) {
  auto vrf = static_cast<RouterID>(dyn[kVrf].asInt());
  ForwardingInformationBaseContainerFields fields{vrf};
  fields.fibV4 =
      ForwardingInformationBaseV4::fromFollyDynamicLegacy(dyn[kFibV4]);
  fields.fibV6 =
      ForwardingInformationBaseV6::fromFollyDynamicLegacy(dyn[kFibV6]);
  return fields;
}

state::FibContainerFields ForwardingInformationBaseContainerFields::toThrift()
    const {
  state::FibContainerFields fields{};
  fields.vrf() = vrf;
  fields.fibV4() = fibV4->toThrift();
  fields.fibV6() = fibV6->toThrift();
  return fields;
}

ForwardingInformationBaseContainerFields
ForwardingInformationBaseContainerFields::fromThrift(
    state::FibContainerFields const& fields) {
  auto vrf = static_cast<RouterID>(*fields.vrf());
  auto fibContainer = ForwardingInformationBaseContainerFields(vrf);
  fibContainer.fibV4 =
      std::make_shared<ForwardingInformationBaseV4>(*fields.fibV4());
  fibContainer.fibV6 =
      std::make_shared<ForwardingInformationBaseV6>(*fields.fibV6());
  return fibContainer;
}

folly::dynamic ForwardingInformationBaseContainerFields::migrateToThrifty(
    folly::dynamic const& dyn) {
  folly::dynamic newDyn = folly::dynamic::object;
  newDyn[kVrf] = dyn[kVrf].asInt();
  newDyn[kFibV4] =
      ForwardingInformationBaseV4::LegacyBaseBase::migrateToThrifty(
          dyn[kFibV4]);
  newDyn[kFibV6] =
      ForwardingInformationBaseV6::LegacyBaseBase::migrateToThrifty(
          dyn[kFibV6]);
  return newDyn;
}

void ForwardingInformationBaseContainerFields::migrateFromThrifty(
    folly::dynamic& dyn) {
  ForwardingInformationBaseV4::LegacyBaseBase::migrateFromThrifty(dyn[kFibV4]);
  ForwardingInformationBaseV6::LegacyBaseBase::migrateFromThrifty(dyn[kFibV6]);
}

ForwardingInformationBaseContainer::ForwardingInformationBaseContainer(
    RouterID vrf) {
  set<switch_state_tags::vrf>(vrf);
}

ForwardingInformationBaseContainer::~ForwardingInformationBaseContainer() {}

RouterID ForwardingInformationBaseContainer::getID() const {
  return RouterID(get<switch_state_tags::vrf>()->cref());
}

const std::shared_ptr<ForwardingInformationBaseV4>&
ForwardingInformationBaseContainer::getFibV4() const {
  return this->cref<switch_state_tags::fibV4>();
}
const std::shared_ptr<ForwardingInformationBaseV6>&
ForwardingInformationBaseContainer::getFibV6() const {
  return this->cref<switch_state_tags::fibV6>();
}

std::shared_ptr<ForwardingInformationBaseContainer>
ForwardingInformationBaseContainer::fromFollyDynamicLegacy(
    const folly::dynamic& json) {
  auto fields =
      ForwardingInformationBaseContainerFields::fromFollyDynamicLegacy(json);
  return std::make_shared<ForwardingInformationBaseContainer>(
      fields.toThrift());
}

std::shared_ptr<ForwardingInformationBaseContainer>
ForwardingInformationBaseContainer::fromFollyDynamic(
    const folly::dynamic& json) {
  auto fields =
      ForwardingInformationBaseContainerFields::fromFollyDynamic(json);
  return std::make_shared<ForwardingInformationBaseContainer>(
      fields.toThrift());
}

folly::dynamic ForwardingInformationBaseContainer::toFollyDynamicLegacy()
    const {
  auto fields =
      ForwardingInformationBaseContainerFields::fromThrift(this->toThrift());
  return fields.toFollyDynamicLegacy();
}

folly::dynamic ForwardingInformationBaseContainer::toFollyDynamic() const {
  auto fields =
      ForwardingInformationBaseContainerFields::fromThrift(this->toThrift());
  return fields.toFollyDynamic();
}

ForwardingInformationBaseContainer* ForwardingInformationBaseContainer::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  auto fibMap = (*state)->getFibs()->modify(state);
  auto newFibContainer = clone();

  auto* rtn = newFibContainer.get();
  fibMap->updateForwardingInformationBaseContainer(std::move(newFibContainer));

  return rtn;
}

template class ThriftStructNode<
    ForwardingInformationBaseContainer,
    state::FibContainerFields>;

} // namespace facebook::fboss
