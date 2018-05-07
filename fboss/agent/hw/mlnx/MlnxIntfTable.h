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

#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>

#include <memory>

extern "C" {
#include <sx/sdk/sx_router.h>
}

namespace folly {
class IPAddress;
}

namespace facebook { namespace fboss {

class Interface;

class MlnxSwitch;
class MlnxIntf;

class MlnxIntfTable {
 public:
  using InterfaceMapT =
    boost::container::flat_map<InterfaceID, std::unique_ptr<MlnxIntf>>;
  using IpToInterfaceMapT =
    boost::container::flat_map<folly::IPAddress, MlnxIntf*>;
  using RifToInterfaceIdMapT =
    boost::container::flat_map<sx_router_interface_t, InterfaceID>;

  /**
   * ctor
   *
   * @param hw Pointer to MlnxSwitch
   */
  MlnxIntfTable(MlnxSwitch* hw);

  /**
   * dtor, all interfaces are deleted from HW
   */
  ~MlnxIntfTable();

  /**
   * add new interface to HW and insert into InterfaceMap
   *
   * @param swIntf Software interface
   */
  void addIntf(const std::shared_ptr<Interface> swIntf);

  /**
   * modify existing interface
   *
   * @param oldIntf Old software interface
   * @param newIntf New software interface
   */
  void changeIntf(const std::shared_ptr<Interface> oldIntf,
    const std::shared_ptr<Interface> newIntf);

  /**
   * delete an interface from HW and remove from InterfaceMap
   *
   * @param swIntf Software interface to delete
   */
  void deleteIntf(const std::shared_ptr<Interface> swIntf);

  /**
   * Returns interface with @intfIp address
   *
   * @param intfIp Interface ip address
   * @ret Pointer to interface object, nullptr if not found
   */
  MlnxIntf* getMlnxIntfByAttachedIpIf(const folly::IPAddress& intfIp) const;

  /**
   * Returns interface Id by sdk rif id
   *
   * @param rif
   * @ret InterfaceId id
   */
  InterfaceID getInterfaceIdByRif(sx_router_interface_t rif) const;

  /**
   * Returns ip -> interface map pointer
   *
   * @ret ip to interface map
   */
  IpToInterfaceMapT* writableIpToInterfaceMap();

  /**
   * Return pointer to MlnxIntf with id @id
   *
   * @param id Interface ID
   * @ret pointer to MlnxIntf
   */
  const MlnxIntf* getMlnxIntfById(InterfaceID id) const;

  /**
   * Returns an iterator to go over interface map
   *
   * @ret iterator pointing to beginning
   */
  InterfaceMapT::const_iterator begin() const;

  /**
   * Returns an interator pointing to end of interface map
   *
   * @ret iterator pointing to end
   */
  InterfaceMapT::const_iterator end() const;
 private:
  // forbidden copy constructor and assignment operator
  MlnxIntfTable(const MlnxIntfTable&) = delete;
  MlnxIntfTable& operator=(const MlnxIntfTable&) = delete;

  // private fields

  MlnxSwitch* hw_ {nullptr};
  InterfaceMapT intfs_ {};
  IpToInterfaceMapT ipToIntf_ {};
  RifToInterfaceIdMapT rifToId_ {};
};

}} // facebook::fboss
