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
#include <fboss/agent/hw/mlnx/MlnxPort.h>
#include <fboss/agent/hw/mlnx/MlnxSwitch.h>
#include <fboss/agent/hw/mlnx/MlnxPlatformPort.h>
#include <fboss/agent/hw/mlnx/MlnxError.h>

#include <fboss/agent/state/Port.h>
#include <fboss/agent/FbossError.h>

#include <folly/Format.h>

extern "C" {
#include <sx/sdk/sx_api_port.h>
#include <sx/sdk/sx_api_vlan.h>
}

namespace {
constexpr auto kJumboFrameSize = 9 * 1024; // 9KiB
}

namespace facebook { namespace fboss {

const std::map<uint32_t, cfg::PortSpeed> MlnxPort::bitmapToSpeedMap = {
   {SPEED_BMAP_1G, cfg::PortSpeed::GIGE},
   {SPEED_BMAP_10G, cfg::PortSpeed::XG},
   {SPEED_BMAP_20G, cfg::PortSpeed::TWENTYG},
   {SPEED_BMAP_25G, cfg::PortSpeed::TWENTYFIVEG},
   {SPEED_BMAP_40G, cfg::PortSpeed::FORTYG},
   {SPEED_BMAP_50G, cfg::PortSpeed::FIFTYG},
   {SPEED_BMAP_100G, cfg::PortSpeed::HUNDREDG},
   {SPEED_BMAP_AUTO, cfg::PortSpeed::DEFAULT},
 };

MlnxPort::MlnxPort(MlnxSwitch* hw,
  PortID id,
  sx_port_log_id_t logicalPort) :
  hw_(hw),
  handle_(hw_->getHandle()),
  logicalPort_(logicalPort),
  portId_(id) {
  sx_status_t rc {SX_STATUS_ERROR};
  sx_port_capability_t port_cap;

  // get the port's supported speed modes
  rc = sx_api_port_capability_get(handle_, logicalPort_, &port_cap);
  mlnxCheckSdkFail(rc,
    "sx_api_port_capability_get",
    "Failed to get port capabilities on port ",
    portId_,
    " (logicalPort ",
    logicalPort,
    ")");

  supportedSpeeds_ = capabilityToPortSpeeds(port_cap.speed_capability);
  maxSpeed_ = *(std::max_element(supportedSpeeds_.begin(),
    supportedSpeeds_.end()));
}

void MlnxPort::init(bool warmBoot) {
  if(!warmBoot) {
    setAdminState(cfg::PortState::DISABLED);
    setMtu(kJumboFrameSize);
    setSpeed(cfg::PortSpeed::DEFAULT);
    setIngressVlanFilter(true);
  } else {
  }

  pltfPort_->linkSpeedChanged(getSpeed());
  pltfPort_->linkStatusChanged(isUp(), isEnabled());
}

void MlnxPort::enable(const std::shared_ptr<Port>& swPort) {
  sx_status_t rc;
  sx_vid_t vid;
  sx_vlan_ports_t vlan_port;
  if (isEnabled()) {
    // already enabled
    return;
  }

  // on enabling disabled port ensure that
  // we have aligned configuration with swPort

  if (getSpeed() != swPort->getSpeed()) {
    setSpeed(swPort->getSpeed());
  }

  if (getIngressVlan() != swPort->getIngressVlan()) {
    setIngressVlan(swPort->getIngressVlan());
  }

  if (getFECMode() != swPort->getFEC()) {
    setFECMode(swPort->getFEC());
  }

  setAdminState(cfg::PortState::ENABLED);
  pltfPort_->linkStatusChanged(isUp(), true);
}

void MlnxPort::disable() {
  sx_status_t rc;
  if (!isEnabled()) {
    return;
  }
  setAdminState(cfg::PortState::DISABLED);
  pltfPort_->linkStatusChanged(false, false);
}

bool MlnxPort::isEnabled() const {
  auto states = getPortStates();

  return states.admin_state !=
      SX_PORT_ADMIN_STATUS_DOWN ? true: false;
}

bool MlnxPort::isUp() const {
  auto states = getPortStates();

  if (states.admin_state != SX_PORT_ADMIN_STATUS_DOWN) {
    return states.oper_state == SX_PORT_OPER_STATUS_UP;
  } else {
    return false;
  }
}

void MlnxPort::setIngressVlanFilter(bool filterEnabled=true) {
  sx_status_t rc;

  rc = sx_api_vlan_port_ingr_filter_set(handle_,
    logicalPort_,
    (filterEnabled ? SX_INGR_FILTER_ENABLE : SX_INGR_FILTER_DISABLE));
  mlnxCheckSdkFail(rc,
    "sx_api_vlan_port_ingr_filter_set",
    "Failed to ",
    (filterEnabled ? "set" : "unset"),
    " ingress filtering on ",
    portId_,
    " to ");
}

void MlnxPort::setIngressVlan(VlanID vlanId) {
  sx_status_t rc;
  sx_vid_t vid = static_cast<sx_vid_t>(vlanId);

  rc = sx_api_vlan_port_pvid_set(handle_,
    SX_ACCESS_CMD_ADD,
    logicalPort_,
    vid);
  mlnxCheckSdkFail(rc,
    "sx_api_vlan_port_pvid_set",
    "Failed to set ingress vlan ",
    vlanId,
    " on port ",
    portId_);
}

VlanID MlnxPort::getIngressVlan() {
  sx_status_t rc;
  sx_vid_t curr_vid;

  rc = sx_api_vlan_port_pvid_get(handle_, logicalPort_, &curr_vid);
  mlnxCheckSdkFail(rc,
    "sx_api_vlan_port_pvid_get",
    "Failed to get ingress vlan on port ",
    portId_);

  return VlanID(curr_vid);
}

void MlnxPort::setAdminState(cfg::PortState newAdminState) {
  sx_port_admin_state_t admin_state = newAdminState ==
      cfg::PortState::ENABLED ? SX_PORT_ADMIN_STATUS_UP:
        SX_PORT_ADMIN_STATUS_DOWN;

  sx_status_t rc = sx_api_port_state_set(handle_, logicalPort_, admin_state);
  mlnxCheckSdkFail(rc,
    "sx_api_port_state_set",
    "Failed to set port admin state for port ",
    portId_);
}

void MlnxPort::setSpeed(cfg::PortSpeed newSpeed) {
  sx_status_t rc;
  sx_port_speed_capability_t port_speed_cap;
  std::memset(&port_speed_cap, 0, sizeof(port_speed_cap));

  if (newSpeed != cfg::PortSpeed::DEFAULT) {
    // check if it is supported
    auto speedIt = std::find(supportedSpeeds_.begin(),
      supportedSpeeds_.end(),
      newSpeed);

    if (speedIt == supportedSpeeds_.end()) {
      // speed we are going to set is not supported by the port
      // Fail hard
      std::stringstream supportedSpeedSStream;

      for (auto speed: supportedSpeeds_) {
        supportedSpeedSStream << " " << static_cast<int>(speed) << "G";
      }

      auto logErrorString = folly::sformat(
        "Port {:d} doesn't support speed {:d}G; Supported speeds are:{}",
        static_cast<int>(portId_),
        static_cast<int>(newSpeed),
        supportedSpeedSStream.str());

      throw FbossError(logErrorString);
    }
  }

  // find capability bitmap from newSpeed
  auto speedEntryIt = std::find_if(bitmapToSpeedMap.begin(),
    bitmapToSpeedMap.end(),
    [newSpeed] (const decltype(*(bitmapToSpeedMap.begin()))& entry) -> bool {
      if (entry.second == newSpeed) {
        return true;
      }
      return false;
    });

  if (speedEntryIt == bitmapToSpeedMap.end()) {
    throw FbossError("Unsupported speed value: ",
      static_cast<int>(newSpeed),
      " on port: ", portId_);
  }

  auto newSpeedCap = speedEntryIt->first;
  bitmapToSpeedCapability(newSpeedCap, port_speed_cap);

  bool portWasEnabled = isEnabled();

  // disable if enabled to ensure
  // the speed would be set to hardware
  if (portWasEnabled) {
    if (isUp()) {
      LOG(WARNING) << "Changing speed on up port. This will "
                   << "disrupt traffic. Port: " << portId_;
    }
    setAdminState(cfg::PortState::DISABLED);
  }

  rc = sx_api_port_speed_admin_set(handle_, logicalPort_, &port_speed_cap);
  mlnxCheckSdkFail(rc,
    "sx_api_port_speed_admin_set",
    "Failed to set admininstative speed");

  if (portWasEnabled) {
    setAdminState(cfg::PortState::ENABLED);
  }

  pltfPort_->linkSpeedChanged(newSpeed);
}

cfg::PortSpeed MlnxPort::getSpeed() const {
  sx_status_t rc;
  sx_port_oper_speed_t oper_speed;
  sx_port_speed_capability_t port_speed_cap;

  rc = sx_api_port_speed_get(handle_, logicalPort_, &port_speed_cap, &oper_speed);
  mlnxCheckSdkFail(rc, "sx_api_port_speed_get");

  uint32_t speedBmap = speedCapabilityToBitmap(port_speed_cap);
  auto speedMapIt = bitmapToSpeedMap.find(speedBmap);

  if (speedMapIt == bitmapToSpeedMap.end()) {
    auto logErrorStr = folly::sformat(
      "Speed set on port {:} is not supported by FBOSS. SDK speed bitmap 0x{:x}",
      static_cast<int>(portId_),
      speedBmap);
    throw FbossError(logErrorStr);
  }

  return speedMapIt->second;
}

cfg::PortSpeed MlnxPort::getMaxSpeed() const {
  return maxSpeed_;
}

MlnxPort::MlnxPortStates MlnxPort::getPortStates() const {
  sx_status_t rc;
  MlnxPort::MlnxPortStates ret;
  sx_port_oper_state_t oper_state;
  sx_port_admin_state_t admin_state;
  sx_port_module_state_t module_state;

  rc = sx_api_port_state_get(handle_, logicalPort_,
          &oper_state, &admin_state, &module_state);
  mlnxCheckSdkFail(rc,
    "sx_api_port_state_get",
    "Cannot get state info on port ",
    portId_,
    " (log port ",
    static_cast<int>(logicalPort_),
    ")");

  ret.oper_state = oper_state;
  ret.admin_state = admin_state;
  ret.module_state = module_state;

  return ret;
}

void MlnxPort::setMtu(uint32_t mtu) {
  sx_status_t rc;
  auto mtu_size = static_cast<sx_port_mtu_t>(mtu);

  rc = sx_api_port_mtu_set(handle_, logicalPort_, mtu_size);
  mlnxCheckSdkFail(rc,
    "sx_api_port_mtu_set"
    "Failed to set MTU ",
    mtu_size,
    " on port ",
    portId_,
    " rc ",
    rc);
}

uint32_t MlnxPort::getMtu() {
  sx_status_t rc;
  sx_port_mtu_t max_mtu;
  sx_port_mtu_t oper_mtu;

  rc = sx_api_port_mtu_get(handle_, logicalPort_, &max_mtu, &oper_mtu);
  mlnxCheckSdkFail(rc,
    "sx_api_port_mtu_get"
    "Failed to get MTU on port ",
    portId_,
    " rc ",
    rc);

  return static_cast<uint32_t>(oper_mtu);
}

void MlnxPort::setFECMode(cfg::PortFEC fec) {
  sx_status_t rc;
  sx_port_phy_mode_t phy_mode;

  phy_mode.fec_mode =
    (fec == cfg::PortFEC::ON ? SX_PORT_FEC_MODE_AUTO : SX_PORT_FEC_MODE_NONE);

  for (auto speed: {
    SX_PORT_PHY_SPEED_10GB,
    SX_PORT_PHY_SPEED_25GB,
    SX_PORT_PHY_SPEED_40GB,
    SX_PORT_PHY_SPEED_50GB,
    SX_PORT_PHY_SPEED_100GB}) {
    rc = sx_api_port_phy_mode_set(handle_,
      logicalPort_,
      speed,
      phy_mode);
    mlnxCheckSdkFail(rc,
     "sx_api_port_phy_mode_set",
     "Failed to set FEC mode ",
     static_cast<int>(fec),
     " on port ",
     portId_);
  }
}

cfg::PortFEC MlnxPort::getFECMode() {
  sx_status_t rc;
  sx_port_phy_mode_t phy_mode_admin;
  sx_port_phy_mode_t phy_mode_oper;

  for (auto speed: {
    SX_PORT_PHY_SPEED_10GB,
    SX_PORT_PHY_SPEED_25GB,
    SX_PORT_PHY_SPEED_40GB,
    SX_PORT_PHY_SPEED_50GB,
    SX_PORT_PHY_SPEED_100GB}) {
    rc = sx_api_port_phy_mode_get(handle_,
      logicalPort_,
      speed,
      &phy_mode_admin,
      &phy_mode_oper);
    mlnxCheckSdkFail(rc,
      "sx_api_port_phy_mode_get",
      "Failed to get FEC setting on port ",
      portId_);
    if (phy_mode_admin.fec_mode == SX_PORT_FEC_MODE_NONE) {
      return cfg::PortFEC::OFF;
    }
  }

  return cfg::PortFEC::ON;
}

cfg::PortFEC MlnxPort::getOperFECMode() {
  sx_status_t rc;
  sx_port_phy_mode_t phy_mode_admin;
  sx_port_phy_mode_t phy_mode_oper;
  sx_port_oper_speed_t oper_speed;
  sx_port_phy_speed_t speed;

  rc = sx_api_port_speed_get(handle_,
    logicalPort_,
    nullptr,
    &oper_speed);
  mlnxCheckSdkFail(rc,
    "sx_api_port_speed_get",
    "Failed to get operative speed on port ",
    portId_);

  switch(oper_speed) {
  case SX_PORT_SPEED_10GB_CX4_XAUI:
  case SX_PORT_SPEED_10GB_KX4:
  case SX_PORT_SPEED_10GB_KR:
  case SX_PORT_SPEED_10GB_CR:
  case SX_PORT_SPEED_10GB_SR:
  case SX_PORT_SPEED_10GB_ER_LR:
    speed = SX_PORT_PHY_SPEED_10GB;
    break;
  case SX_PORT_SPEED_25GB_CR:
  case SX_PORT_SPEED_25GB_KR:
  case SX_PORT_SPEED_25GB_SR:
    speed = SX_PORT_PHY_SPEED_25GB;
    break;
  case SX_PORT_SPEED_40GB_CR4:
  case SX_PORT_SPEED_40GB_KR4:
  case SX_PORT_SPEED_40GB_SR4:
  case SX_PORT_SPEED_40GB_LR4_ER4:
    speed = SX_PORT_PHY_SPEED_40GB;
    break;
  case SX_PORT_SPEED_50GB_CR2:
  case SX_PORT_SPEED_50GB_KR2:
  case SX_PORT_SPEED_50GB_SR2:
    speed = SX_PORT_PHY_SPEED_50GB;
    break;
  case SX_PORT_SPEED_100GB_LR4_ER4:
  case SX_PORT_SPEED_100GB_CR4:
  case SX_PORT_SPEED_100GB_SR4:
  case SX_PORT_SPEED_100GB_KR4:
    speed = SX_PORT_PHY_SPEED_100GB;
    break;
  default:
    // Other speed not supported
    return cfg::PortFEC::OFF;
  }

  rc = sx_api_port_phy_mode_get(handle_,
    logicalPort_,
    speed,
    &phy_mode_admin,
    &phy_mode_oper);
  mlnxCheckSdkFail(rc,
    "sx_api_port_phy_mode_get",
    "Failed to get FEC setting on port ",
    portId_);

  if (phy_mode_oper.fec_mode == SX_PORT_FEC_MODE_NONE) {
    return cfg::PortFEC::OFF;
  }

  return cfg::PortFEC::ON;
}

std::vector<cfg::PortSpeed> MlnxPort::capabilityToPortSpeeds(
        const sx_port_speed_capability_t& capability) const {
  std::vector<cfg::PortSpeed> supportedSpeeds{};
  auto speedCapabilityBitmap = speedCapabilityToBitmap(capability);

  for (const auto& speedEntry: bitmapToSpeedMap) {
    auto speedBitmap = speedEntry.first;
    if (speedBitmap & speedCapabilityBitmap) {
      supportedSpeeds.push_back(speedEntry.second);
    }
  }

  return supportedSpeeds;
}

void MlnxPort::setPortPauseSetting(cfg::PortPause pause) {
  sx_status_t rc;
  sx_port_flow_ctrl_mode_t fc_mode;

  if (pause.rx && pause.tx) {
    fc_mode = SX_PORT_FLOW_CTRL_MODE_TX_EN_RX_EN;
  } else if (pause.rx && !pause.tx) {
    fc_mode = SX_PORT_FLOW_CTRL_MODE_TX_DIS_RX_EN;
  } else if (!pause.rx && pause.tx) {
    fc_mode = SX_PORT_FLOW_CTRL_MODE_TX_EN_RX_DIS;
  } else {
    fc_mode = SX_PORT_FLOW_CTRL_MODE_TX_DIS_RX_DIS;
  }

  rc = sx_api_port_global_fc_enable_set(handle_,
    logicalPort_,
    fc_mode);
  mlnxCheckSdkFail(rc,
    "sx_api_port_global_fc_enable_set",
    "Failed to set port pause settings on port ",
    portId_);
}

cfg::PortPause MlnxPort::getPortPauseSetting() {
  sx_status_t rc;
  sx_port_flow_ctrl_mode_t fc_mode;
  cfg::PortPause pause;

  rc = sx_api_port_global_fc_enable_get(handle_,
    logicalPort_,
    &fc_mode);
  mlnxCheckSdkFail(rc,
    "sx_api_port_global_fc_enable_set",
    "Failed to set port pause settings on port ",
    portId_);

  if (fc_mode == SX_PORT_FLOW_CTRL_MODE_TX_EN_RX_EN) {
    pause.rx = true;
    pause.tx = true;
  } else if (fc_mode == SX_PORT_FLOW_CTRL_MODE_TX_DIS_RX_EN) {
    pause.rx = true;
    pause.tx = false;
  } else if (fc_mode == SX_PORT_FLOW_CTRL_MODE_TX_EN_RX_DIS) {
    pause.rx = false;
    pause.tx = true;
  } else {
    pause.rx = false;
    pause.tx = false;
  }

  return pause;
}

uint32_t MlnxPort::speedCapabilityToBitmap(
  const sx_port_speed_capability_t& capability) const {
  uint32_t bmap = 0;
  const boolean_t* ptr = (const boolean_t*)&capability;

  const uint8_t capability_size = sizeof(capability) / sizeof(boolean_t);

  // go through the SDK structure and generate bitmap
  for (int index = 0; index < capability_size; ++index) {
    bmap |= ptr[index] << index;
  }
  return bmap;
}

void MlnxPort::bitmapToSpeedCapability(uint32_t bmap,
  sx_port_speed_capability& capability) const {
  boolean_t* ptr = (boolean_t*)&capability;

  const uint8_t capability_size = sizeof(capability) / sizeof(boolean_t);

  // go through the bitmap and generate SDK structure
  for (int index = 0; index < capability_size; ++index) {
    ptr[index] = (bmap & (0x1 << index)) ? 1 : 0;
  }
}

void MlnxPort::setPlatformPort(MlnxPlatformPort* pltfPort) {
  pltfPort_ = pltfPort;
}

MlnxPlatformPort* MlnxPort::getPlatformPort() {
  return pltfPort_;
}

}}
