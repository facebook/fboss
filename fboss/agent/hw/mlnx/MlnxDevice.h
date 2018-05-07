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

#include "fboss/agent/hw/mlnx/MlnxConfig.h"

extern "C" {
#include <sx/sdk/sx_api.h>
#include <sx/sdk/sx_port.h>
#include <sx/sdk/sx_dev.h>
}

#include <atomic>
#include <vector>

namespace folly {
class MacAddress;
}

namespace facebook { namespace fboss {

class MlnxSwitch;

/*
 * Mellanox device abstraction.
 * Performs init and deinit of a device
 * including the port mapping.
 */
class MlnxDevice {
 public:
  /**
   * Constructs the MlnxDevice object
   * Any use after init() call
   *
   * @param hw MlnxSwitch
   * @param device DeviceInfo structure from xml configuration
   */
  MlnxDevice(MlnxSwitch* hw, const MlnxConfig::DeviceInfo& device);

  /**
   * dtor
   */
  ~MlnxDevice();

  /**
   * Initialize device to SDK
   */
  void init();

  /**
   * Deinitialize device from SDK
   */
  void deinit();

  /**
   * Returns port attributes
   *
   * @ret vector of port attributes
   */
  const std::vector<sx_port_attributes_t>& getPortAttrs() const;

 private:
  MlnxDevice(const MlnxDevice&) = delete;
  MlnxDevice& operator=(const MlnxDevice&) = delete;

  /**
   * Applies port mapping from device config
   */
  void applyPortMapping();

  // private fields

  // has been initialized flag
  std::atomic<bool> initialized_{false};

  MlnxSwitch* hw_;
  // SDK handle
  sx_api_handle_t handle_ {SX_API_INVALID_HANDLE};

  // device configuration structure
  const MlnxConfig::DeviceInfo& deviceCfg_;
  // device id
  sx_device_id_t deviceId_;
  // device base mac address
  sx_mac_addr_t baseMacAddr_;
  // port attributes list
  std::vector<sx_port_attributes_t> portAttrs_;
};

}} /* namespace facebook::fboss */
