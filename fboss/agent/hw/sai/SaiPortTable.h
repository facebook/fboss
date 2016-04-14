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

#include "fboss/agent/types.h"

extern "C" {
#include "saitypes.h"
}

namespace facebook { namespace fboss {

class SaiPortBase;
class SaiSwitch;

class SaiPortTable {
public:
  explicit SaiPortTable(SaiSwitch *hw);
  virtual ~SaiPortTable();

  /*
   * Initialize the port table from the list of physical switch ports.
   *
   * No other SaiPortTable methods should be accessed before initPorts()
   * completes.
   */
  void initPorts(bool warmBoot);

  /*
  * Getters.
  */
  sai_object_id_t getSaiPortId(PortID id) const; 
  PortID getPortId(sai_object_id_t portId) const;

  // Throw an error if not found
  SaiPortBase *getSaiPort(PortID id) const;
  SaiPortBase *getSaiPort(sai_object_id_t id) const;

  /*
   * Indicate that a port's link status has changed.
   */
  void setPortStatus(sai_object_id_t portId, int status);

  /*
   * Update all ports' statistics.
   */
  void updatePortStats();

private:
  typedef boost::container::flat_map<sai_object_id_t, std::unique_ptr<SaiPortBase>> SaiPortMap;
  typedef boost::container::flat_map<PortID, SaiPortBase *> FbossPortMap;

  SaiSwitch *hw_ {nullptr};

  // Mappings for the physical ports.
  // The set of physical ports is defined in initPorts(), and is read-only
  // afterwards (since the physical ports on the switch cannot change).
  // Therefore we don't need any locking on this data structure.
  // (Modifiable data in the SaiPort objects themselves does require locking,
  // though.)

  // A mapping from sai_object_id_t to SaiPort.
  SaiPortMap saiPhysicalPorts_;
  // A mapping from FBOSS PortID to SaiPort.
  FbossPortMap fbossPhysicalPorts_;
};

}} // facebook::fboss
