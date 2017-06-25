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

#include <boost/container/flat_set.hpp>

namespace facebook { namespace fboss {

struct PortAndEgressIdsFields {
  typedef boost::container::flat_set<opennsl_if_t> EgressIds;
  PortAndEgressIdsFields(opennsl_if_t port, EgressIds egressIds) :
    id(port), egressIds(std::move(egressIds)) {}

  template<typename Fn>
  void forEachChild(Fn fn) {}

  const opennsl_port_t id{0};
  EgressIds egressIds;
};

/*
 * Port and set of corresponding egress Ids
 */
class PortAndEgressIds: public NodeBaseT<PortAndEgressIds,
  PortAndEgressIdsFields> {
 public:
    typedef PortAndEgressIdsFields::EgressIds EgressIds;
    PortAndEgressIds(opennsl_if_t port, EgressIds egressIds) :
      NodeBaseT(port, std::move(egressIds)) {}

  opennsl_if_t getID() const {
    return getFields()->id;
  }
  const EgressIds& getEgressIds() const {
    return getFields()->egressIds;
  }

  bool empty() const {
    return getEgressIds().size() == 0;
  }
  void setEgressIds(EgressIds egressIds) {
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

  static std::shared_ptr<PortAndEgressIds>
    fromFollyDynamic(const folly::dynamic& json) {
    CHECK(0); // Not needed yet
    return std::make_shared<PortAndEgressIds>(0, EgressIds());
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

typedef NodeMapTraits<opennsl_port_t, PortAndEgressIds>
PortAndEgressIdsMapTraits;
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
      opennsl_port_t port) const {
    return getNode(port);
  }
  /*
   * Get the PortAndEgressIds for a given port.
   *
   * Throws an FbossError if the mapping does not exist.
   */
  std::shared_ptr<PortAndEgressIds> getPortAndEgressIdsIf(
      opennsl_port_t port) const {
    return getNodeIf(port);
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

  void removePort(opennsl_port_t port) {
    removeNode(port);
  }

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;

};

}}
