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

extern "C" {
#include <opennsl/types.h>
}

#include <folly/dynamic.h>
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/hw/bcm/BcmEgress.h"

#include <boost/container/flat_set.hpp>

namespace facebook { namespace fboss {

struct PortAndEgressIdsFields {
  using EgressIdSet = BcmEcmpEgress::EgressIdSet;
  PortAndEgressIdsFields(opennsl_gport_t gport, EgressIdSet egressIds)
      : id(gport), egressIds(std::move(egressIds)) {}

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  const opennsl_gport_t id{0};
  EgressIdSet egressIds;
};

/*
 * Port and set of corresponding egress Ids
 */
class PortAndEgressIds: public NodeBaseT<PortAndEgressIds,
  PortAndEgressIdsFields> {
 public:
    typedef PortAndEgressIdsFields::EgressIdSet EgressIdSet;
    PortAndEgressIds(opennsl_gport_t gport, EgressIdSet egressIds)
        : NodeBaseT(gport, std::move(egressIds)) {}

    opennsl_gport_t getID() const {
      return getFields()->id;
  }
  const EgressIdSet& getEgressIds() const {
    return getFields()->egressIds;
  }

  bool empty() const {
    return getEgressIds().size() == 0;
  }
  void setEgressIds(EgressIdSet egressIds) {
    writableFields()->egressIds = std::move(egressIds);
  }

  void addEgressId(opennsl_if_t egressId) {
    writableFields()->egressIds.insert(egressId);
  }

  void removeEgressId(opennsl_if_t egressId) {
    writableFields()->egressIds.erase(egressId);
  }

  folly::dynamic toFollyDynamic() const override {
    CHECK(0); // Not needed yet
    return folly::dynamic::object;
  }

  static std::shared_ptr<PortAndEgressIds> fromFollyDynamic(
      const folly::dynamic& /*json*/) {
    CHECK(0); // Not needed yet
    return std::make_shared<PortAndEgressIds>(0, EgressIdSet());
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

using PortAndEgressIdsMapTraits =
    NodeMapTraits<opennsl_gport_t, PortAndEgressIds>;

/*
 * Container for maintaining mapping from port to
 * egressIds
 */
class PortAndEgressIdsMap: public NodeMapT<PortAndEgressIdsMap,
  PortAndEgressIdsMapTraits> {
 public:
   PortAndEgressIdsMap();
   ~PortAndEgressIdsMap() override;
  /*
   * Get the PortAndEgressIds for a  given port.
   *
   * Throws an FbossError if the mapping does not exist.
   */
   const std::shared_ptr<PortAndEgressIds>& getPortAndEgressIds(
       opennsl_gport_t gport) const {
     return getNode(gport);
  }
  /*
   * Get the PortAndEgressIds for a given port.
   *
   * Throws an FbossError if the mapping does not exist.
   */
  std::shared_ptr<PortAndEgressIds> getPortAndEgressIdsIf(
      opennsl_gport_t gport) const {
    return getNodeIf(gport);
  }
  /*
   * The following functions modify the object state.
   * These should only be called on unpublished objects which
   * are only visible to a single thread.
   */

  void addPortAndEgressIds(const std::shared_ptr<PortAndEgressIds>&
      portAndEgressIds) {
    addNode(portAndEgressIds);
  }

  void updatePortAndEgressIds(const std::shared_ptr<PortAndEgressIds>&
      portAndEgressIds) {
    updateNode(portAndEgressIds);
  }

  void removePort(opennsl_gport_t gport) {
    removeNode(gport);
  }

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;

};

}}
