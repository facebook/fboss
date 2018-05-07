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

#include "fboss/agent/state/Interface.h"

extern "C" {
#include <sx/sdk/sx_api.h>
#include <sx/sdk/sx_router.h>
}

namespace folly {
class IPAddress;
}

namespace facebook { namespace fboss {

class MlnxSwitch;
class MlnxIntfTable;

/**
 * MlnxIntf abstracts operations with L3 VLAN interface in HW
 */
class MlnxIntf {
 public:

  /**
   * ctor, once the object is created it is programmed to HW.
   * So during the lifetime of MlnxIntf instance we are sure
   * that L3 Interface exists in SDK
   *
   * @param hw Pointer to MlnxSwitch
   * @param intfTable Pointer to MlnxIntfTable
   * @param swInf Reference to software interface object to align with
   */
  MlnxIntf(MlnxSwitch* hw, MlnxIntfTable* intfTable, const Interface& swIntf);

  /**
   * dtor, deletes all configuration from HW
   */
  ~MlnxIntf();

  /**
   * This methods edits configuration on L3 interface
   * NOTE: Only mac and mtu can be changed in runtime
   *
   * @param newIntf Modified software interface
   */
  void change(const Interface& newIntf);

  /**
   * Returns SDK RIF
   *
   * @ret RIF
   */
  sx_router_interface_t getSdkRif() const;

  /**
   * Serialize interface configuration into string
   * Useful for logging and debugging
   *
   * @ret serialized interface configuration
   */
  std::string toString() const;

 private:
  MlnxIntf() = default;

  // private methods

  /**
   * Method to align HW interface configuration with SW interface
   *
   * @param swInf Reference to software interface object to align with
   * @param cmd add or edit (SX_ACCES_CMD_ADD or SX_ACCESS_CMD_EDIT)
   */
  void program(const Interface& swIntf, sx_access_cmd_t cmd);

  /**
   * Programs IP2ME route, so packets destinated to the interface
   * are trapped to CPU
   *
   * @param ip IP address attached to interface
   * @param cmd add or delete (SX_ACCES_CMD_ADD or SX_ACCESS_CMD_DELETE)
   */
  void programIP2ME(const folly::IPAddress& ip, sx_access_cmd_t cmd);

  /**
   * This method creates and binds
   * router interface counters to the interface
   *
   * Used only for debugging
   */
  void createAndBindCounter();

  /**
   * This method ubinds and destroys a router counter
   * created by createAndBindCounter() method
   */
  void unbindAndDestroyCounter();

  /**
   * Set state for interface
   *
   * @param up True if need to enable, otherwise disable
   */
  void setAdminState(bool up);

  // private fields

  // SDK handle
  sx_api_handle_t handle_ {SX_API_INVALID_HANDLE};
  // Pointer to MlnxSwitch
  MlnxSwitch* hw_ {nullptr};
  MlnxIntfTable* intfTable_ {nullptr};
  // Set of attached interface IPs
  Interface::Addresses interfaceIpSet_;
  // FBOSS  Interface ID
  InterfaceID id_;
  // SDK counter id (counters mainly used for debugging)
  sx_router_counter_id_t counter_;

  // Interface configuration structures
  sx_router_interface_param_t interfaceParams_ {};
  sx_interface_attributes_t interfaceAttrs_ {};

  // virtual router id in SDK
  sx_router_interface_t vrid_;
  // router id in SDK
  sx_router_interface_t rif_;
};

}} // facebook::fboss
