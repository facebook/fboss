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

#include "fboss/agent/hw/mlnx/MlnxPlatform.h"
#include "fboss/agent/types.h"

#include <fboss/agent/PlatformPort.h>

namespace facebook { namespace fboss {

/**
 * This class contains mostly platform specific port functionality.
 * (For now most of methods have stub implementation)
 */
class MlnxPlatformPort : public PlatformPort {
 public:

  /**
   * ctor
   *
   * @param platform Pointer to MlnxPlatform
   * @param portId Port Id
   * @param frontPanelPort
   * @param channel Channel ID
   */
  MlnxPlatformPort(MlnxPlatform* platform,
    PortID portId,
    folly::Optional<TransceiverID> frontPanelPort = folly::none,
    std::vector<int32_t> channels = {});

  /**
   * dtor
   */
  virtual ~MlnxPlatformPort() override = default;

  // default move ctor and move assignment
  MlnxPlatformPort(MlnxPlatformPort&&) = default;
  MlnxPlatformPort& operator=(MlnxPlatformPort&&) = default;

  /**
   * Get the PortID for this port
   */
  PortID getPortID() const override;

  /**
   * preDisable() will be called by the hardware code just before disabling
   * a port.
   *
   * @param temporary Will be true if the port is configured to be
   * enabled, but is only being disabled temporarily to make some settings
   * changes.  (For instance, to change the port speed or interface type.)
   */
  void preDisable(bool temporary) override;

  /*
   * postDisable() will be called by the hardware code just after disabling a
   * port.
   *
   * @param temporary Will be true if the port is configured to be
   * enabled, but is only being disabled temporarily to make some settings
   * changes.  (For instance, to change the port speed or interface type.)
   */
  void postDisable(bool temporary) override;

  /*
   * preEnable() will be called by the hardware code just before enabling
   * a port.
   */
  void preEnable() override;

  /*
   * postEnable() will be called by the hardware code just after enabling
   * a port.
   */
  void postEnable() override;

  /*
   * isMediaPresent() must be implemented by the PlatformPort subclass,
   * and should return if media is present in the port.
   *
   * For instance, for a port connected to an SFP+ module, this should indicate
   * if a module is plugged in.  For an internal backplane port, this should
   * indicate if the remote fabric or line card is present.
   *
   * This is primarily used to indicate if link is expected to be up on the
   * port or not.  If no media is present, it is expected that the link will be
   * down.  If media is present but the link is down, the hardware code may try
   * periodically tuning the port parameters to see if it can achieve link.
   */
  bool isMediaPresent() override;

  /**
   * linkStatusChanged() will be called by the hardware code whenever
   * the link status changed.
   *
   * @param up indicates if link is present.
   * @param adminUp indicates if the port is enabled or disabled.
   */
  void linkStatusChanged(bool up, bool adminUp) override;

  /**
   * Will be called by the hardware code whenever the port speed
   * changes.
   *
   * @param speed indicates the new speed that the port is running at
   */
  void linkSpeedChanged(const cfg::PortSpeed& speed) override;

  /**
   * Returns true if the port supports/expects to use a transceiver
   * and false if there is a direct electrical connection
   */
  bool supportsTransceiver() const override;

  /**
   * Returns the transceiver's id if the port supports a transceiver,
   * otherwise, returns an empty folly::Optional
   *
   * @ret TransceiverID or empty folly::Optional if does not support
   */
  folly::Optional<TransceiverID> getTransceiverID() const override;

  /**
   * statusIndication() will be called by the hardware code once a second.
   *
   * @param enabled  - If the port is enabled or not.
   * @param link     - If link is up on the port.
   * @param ingress  - If any packets have been received on the port since the last
   *                   statusIndication() call.
   * @param egress   - If any packets have been sent on the port since the last
   *                   statusIndication() call.
   * @param discards - If any packets have been discarded on the port since the last
   *                   statusIndication() call.
   * @param errors   - If any packet errors have occurred on the port since the last
   *                   statusIndication() call.
   */
  void statusIndication(bool enabled,
    bool link,
    bool ingress,
    bool egress,
    bool discards,
    bool errors) override;

  /**
   * Do platform specific actions for the port before we are warm booting.
   */
  void prepareForGracefulExit() override;

  /**
   * There are situations (e.g. backplane ports on some platforms)
   * where we don't need to enable FEC for ports even on 100G speeds.
   * Not enabling FEC on these means that we don't incur the (very)
   * minor latency (100ns) that enabling FEC entails.
   * So, provide derived class platform ports a hook to override
   * decision to enable FEC.
   *
   * @ret True if should disable FEC, otherwise false
   */
  bool shouldDisableFEC() const override;

  /**
   * Serialize port status and speed into thrift object.
   * Used to communicate with qsfp service
   *
   * @param port SW port pointer
   * @ret PortStatus serialized port status info
   */
  PortStatus toThrift(const std::shared_ptr<Port>& port);

  /**
   * Returns a vector of channels used by this port.
   * If port does not supports transceiver it should
   * return empty vector
   *
   * @ret veсtor of channels (lane numbers)
   */
  std::vector<int32_t> getChannels() const;

  /**
   * Returns transceiver id in thrift object.
   * Used to communicate with qsfp service
   *
   * @ret transceiver id in thrift object
   */
  TransceiverIdxThrift getTransceiverMapping() const;

 private:
  // Forbidden copy constructor and assignment operator
  MlnxPlatformPort(const MlnxPlatformPort&) = delete;
  MlnxPlatformPort& operator=(const MlnxPlatformPort&) = delete;

  // private fields

  MlnxPlatform* platform_ {nullptr};
  PortID portId_;
  folly::Optional<TransceiverID> frontPanelPort_;
  std::vector<int32_t> channels_;
};

}} // facebook::fboss
