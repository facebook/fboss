/* 
 * Copyright (c) Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "fboss/agent/hw/mlnx/MlnxPort.h"
#include "fboss/agent/state/Vlan.h"

#include <boost/container/flat_map.hpp>

#include <memory>

namespace facebook { namespace fboss {

/*
 * This class reflects the mapping between PortID and MlnxPort
 */
class MlnxPortTable {
 public:
  
  /**
   * ctor, call initPorts() to initialize port table
   */
  explicit MlnxPortTable(MlnxSwitch* hw);

  /**
   * dtor
   */
  ~MlnxPortTable();

  using FbossPortMapT = boost::container::flat_map<PortID,
    std::unique_ptr<MlnxPort>>;
  using LogPortMapT = boost::container::flat_map<sx_port_log_id_t, MlnxPort*>;
  
  /**
   * Initialize ports
   *
   * @param warmBoot if we are warm booting
   */
  void initPorts(bool warmBoot);

  /**
   * Add a set of ports to vlan
   *
   * @param vlanPorts Vlan ports information
   * @param vlan Vlan ID
   */
  void addPortsToVlan(const Vlan::MemberPorts& vlanPorts,
    VlanID vlan);

  /**
   * Remove a set of ports from vlan
   *
   * @param vlanPorts Vlan ports information
   * @param vlan Vlan ID
   */
  void removePortsFromVlan(const Vlan::MemberPorts& vlanPorts,
    VlanID vlan);

  /**
   * Insert port into table
   *
   * @param mlnxPort MlnxPort object
   */
  void insert(std::unique_ptr<MlnxPort>& mlnxPort);

  /**
   * Get log_port by port ID
   * Throw exception if not found
   *
   * @ret log_port
   */
  sx_port_log_id_t getMlnxLogPort(PortID id) const;

  /**
   * Get port ID by log_port
   * Throw exception if not found
   *
   * @ret port ID
   */
  PortID getPortID(sx_port_log_id_t log_port) const;

  /**
   * Get port by port ID
   * Throw exception if not found
   *
   * @ret pointer to MlnxPort object
   */
  MlnxPort* getMlnxPort(PortID id) const;

  /**
   * Get port by log_port
   * Throw exception if not found
   *
   * @ret pointer to MlnxPort object
   */
  MlnxPort* getMlnxPort(sx_port_log_id_t log_port) const;

  // return iterators
  FbossPortMapT::const_iterator begin() const;
  FbossPortMapT::const_iterator end() const;

 private:

  /**
   * Set port to/from vlan
   *
   * @param vlanPorts Vlan ports information
   * @param vlan Vlan ID
   * @param add True if we want to add ports in @vlanPorts to @vlan
   */
  void setVlanPorts(const Vlan::MemberPorts& vlanPorts,
    VlanID vlan,
    bool add);

  MlnxSwitch* hw_{nullptr};

  FbossPortMapT fbossToMlnxPort_;
  LogPortMapT logToMlnxPort_;
};

}}
