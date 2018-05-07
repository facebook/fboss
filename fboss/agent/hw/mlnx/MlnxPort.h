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
#include <fboss/agent/gen-cpp2/switch_config_types.h>

extern "C" {
#include <sx/sdk/sx_api.h>
#include <sx/sdk/sx_port.h>
}

#include <map>

namespace facebook { namespace fboss {

class Port;

class MlnxSwitch;
class MlnxPlatformPort;

/*
 *  This class abstracts logical port in Mellanox Switch
 */
class MlnxPort {
 public:

  /**
   * Represent states of port
   *   - administrative
   *   - operational
   *   - module
   */
  struct MlnxPortStates {
    sx_port_admin_state_t admin_state;
    sx_port_oper_state_t oper_state;
    sx_port_module_state_t module_state;
  };

  /**
   * Speed bitmaps based on
   * sx_port_speed_capabilities_t structure
   */
  enum : uint32_t {
    SPEED_BMAP_1G = 0x3,
    SPEED_BMAP_10G = 0x1c1c,
    SPEED_BMAP_20G = 0x20,
    SPEED_BMAP_25G = 0x380000,
    SPEED_BMAP_40G = 0x60c0,
    SPEED_BMAP_50G = 0x1c00000,
    SPEED_BMAP_100G = 0x78000,
    SPEED_BMAP_AUTO = 0x2000000,
  };

  /**
   * SDK speed bitmap to FBOSS speed mapping
   */
  static const std::map<uint32_t, cfg::PortSpeed> bitmapToSpeedMap;

  /**
   * ctor, initializes port to SDK
   *
   * @param hw pointer to MlnxSwitch
   * @param id FBOSS port id
   * @param logicalPort_ Logical port number
   */
  MlnxPort(MlnxSwitch* hw, PortID id, sx_port_log_id_t logicalPort_);

  /**
   * Get FBOSS port id
   *
   * @return port id
   */
  auto getPortId() { return portId_; }

  /**
   * Get SDK logical port number
   *
   * @return logical port number
   */
  auto getLogPort() { return logicalPort_; }

  /**
   * Perform port initialization
   *
   * @param warmBoot boot type
   */
  void init(bool warmBoot);

  /**
   * Set administrative state up
   *
   * @param swPort Software switch port
   */
  void enable(const std::shared_ptr<Port>& swPort);

  /**
   * Set administrative state down
   *
   */
  void disable();

  /**
   * Set ingress vlan filter on port
   *
   * @param filterEnabled true to enable, false to disable
   */
  void setIngressVlanFilter(bool filterEnabled);

  /**
   * Set ingress vLAN
   *
   * @param vlanId Ingress vlanId
   */
  void setIngressVlan(VlanID vlanId);

  /**
   * Get ingress vLAN (PVID)
   *
   * @param vlanId Ingress vlanId
   */
  VlanID getIngressVlan();

  /**
   * Check if administrative state is up
   *
   * @return true if up
   */
  bool isEnabled() const;

  /**
   * Check if operative state is up
   *
   * @return true if up
   */
  bool isUp() const;

  /**
   * Set MTU value
   *
   * @param mtu Maximum transmission unit
   */
  void setMtu(uint32_t mtu);

  /**
   * Return port's MTU
   *
   * @return MTU
   */
  uint32_t getMtu();

  /**
   * Set port's speed
   *
   * @param newSpeed Speed to be set
   */
  void setSpeed(cfg::PortSpeed newSpeed);

  /**
   * Get port speed
   *
   * @return port speed
   */
  cfg::PortSpeed getSpeed() const;

  /**
   * Get the maximum supported speed
   * on the port
   *
   * @return Maximum supported speed
   */
  cfg::PortSpeed getMaxSpeed() const;

  /**
   * Set FEC mode on port
   *
   * @param fec FEC (ON, OFF)
   */
  void setFECMode(cfg::PortFEC fec);

  /**
   * Gets FEC mode set on port
   *
   * @return FEC mode
   */
  cfg::PortFEC getFECMode();

  /**
   * Gets operative FEC mode set on port
   * Is needed for HwSwitch::getPortFecConfig
   *
   * @return FEC mode
   */
  cfg::PortFEC getOperFECMode();

  /**
   * Set pause setting
   *
   * @param Pause configuration
   */
  void setPortPauseSetting(cfg::PortPause pause);

  /**
   * Get pause setting
   *
   * @ret Pause configuration
   */
  cfg::PortPause getPortPauseSetting();

  /**
   * Sets MlnxPlatformPort instance
   * This sould be set on init
   *
   * @param mlnxPltfPort Platform port instance
   */
  void setPlatformPort(MlnxPlatformPort* pltfPort);

  /**
   * Gets MlnxPlatformPort instance associated with this port
   *
   * @return mlnxPltfPort Platform port instance
   */
  MlnxPlatformPort* getPlatformPort();

 private:

  /**
   * Returns port states:
   *   - administrative
   *   - operational
   *   - module
   *
   * @return MlnxPortState structure
   */
  MlnxPortStates getPortStates() const;

  /**
   * Sets administrative state on port
   *
   * @param newAdminState Administrative state to set (Up/Down)
   */
  void setAdminState(cfg::PortState newAdminState);

  /**
   * Converts SDK speed capability structure to the
   * list of cfg::PortSpeed
   *
   * @param capability SDK speed capability structure
   * @return Vector of FBOSS port speeds
   */
  std::vector<cfg::PortSpeed> capabilityToPortSpeeds (
    const sx_port_speed_capability_t& capability) const;

  /**
   * Converts SDK speed capability structure to the
   * 32-bit bitmap
   *
   * @param capability SDK speed capability structure
   * @return capability speed bitmap
   */
  uint32_t speedCapabilityToBitmap(
    const sx_port_speed_capability_t& capability) const;

  /**
   * Converts 32-bit bitmap to SDK speed capability structure
   *
   * @param capability speed bitmap
   * @param capability SDK speed capability structure
   */
  void bitmapToSpeedCapability(uint32_t bmap,
      sx_port_speed_capability&) const;

  MlnxSwitch* hw_ {nullptr};
  sx_api_handle_t handle_{SX_API_INVALID_HANDLE};
  sx_port_log_id_t logicalPort_{SX_SPAN_INVALID_LOG_PORT};
  PortID portId_;
  std::vector<cfg::PortSpeed> supportedSpeeds_{};
  cfg::PortSpeed maxSpeed_;
  MlnxPlatformPort* pltfPort_{nullptr};
};

}}
