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

#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

class SwitchState;

using ForwardingInformationBaseMapTraits =
    NodeMapTraits<RouterID, ForwardingInformationBaseContainer>;

class ForwardingInformationBaseMap : public NodeMapT<
                                         ForwardingInformationBaseMap,
                                         ForwardingInformationBaseMapTraits> {
 public:
  ForwardingInformationBaseMap();
  ~ForwardingInformationBaseMap() override;

  std::pair<uint64_t, uint64_t> getRouteCount() const;

  std::shared_ptr<ForwardingInformationBaseContainer> getFibContainerIf(
      RouterID vrf) const;

  std::shared_ptr<ForwardingInformationBaseContainer> getFibContainer(
      RouterID vrf) const;

  ForwardingInformationBaseMap* modify(std::shared_ptr<SwitchState>* state);

  void updateForwardingInformationBaseContainer(
      const std::shared_ptr<ForwardingInformationBaseContainer>& fibContainer);

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
