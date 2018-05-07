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
#include "fboss/agent/hw/mlnx/MlnxPlatformPort.h"

#include "fboss/agent/state/Port.h"

namespace facebook { namespace fboss {

MlnxPlatformPort::MlnxPlatformPort(MlnxPlatform* platform,
  PortID portId,
  folly::Optional<TransceiverID> frontPanelPort,
  std::vector<int32_t> channels)
  : platform_(platform),
    portId_(portId),
    frontPanelPort_(frontPanelPort),
    channels_(channels) {
}

PortID MlnxPlatformPort::getPortID() const {
  return portId_;
}

void MlnxPlatformPort::preDisable(bool /* temporary */) {
}

void MlnxPlatformPort::postDisable(bool /* temporary */) {
}

void MlnxPlatformPort::preEnable() {
}

void MlnxPlatformPort::postEnable() {
}

bool MlnxPlatformPort::isMediaPresent() {
  LOG(WARNING) << "isMediaPresent() always returns true for now";
  return true;
}

void MlnxPlatformPort::linkStatusChanged(bool /* up */, bool /* adminUp */) {
}

void MlnxPlatformPort::linkSpeedChanged(const cfg::PortSpeed& /* speed */) {
}

bool MlnxPlatformPort::supportsTransceiver() const {
  return getTransceiverID().hasValue();
}

folly::Optional<TransceiverID> MlnxPlatformPort::getTransceiverID() const {
  return frontPanelPort_;
}

void MlnxPlatformPort::statusIndication(bool /* enabled */,
  bool /* link */,
  bool /* ingress */,
  bool /* egress */,
  bool /* discards */,
  bool /* errors */) {
}

void MlnxPlatformPort::prepareForGracefulExit() {
}

bool MlnxPlatformPort::shouldDisableFEC() const {
  LOG(WARNING) << "shouldDisableFEC() always returns false for now";
  return false;
}

PortStatus MlnxPlatformPort::toThrift(const std::shared_ptr<Port>& port) {
  PortStatus status;

  status.enabled = port->isEnabled();
  status.up = port->isUp();

  status.speedMbps = static_cast<int64_t>(port->getSpeed());

  if (supportsTransceiver()) {
    status.set_transceiverIdx(getTransceiverMapping());
  }

  return status;
}

std::vector<int32_t> MlnxPlatformPort::getChannels() const {
  return channels_;
}

TransceiverIdxThrift MlnxPlatformPort::getTransceiverMapping() const {
  if (!supportsTransceiver()) {
    return TransceiverIdxThrift();
  }
  return TransceiverIdxThrift(
    apache::thrift::FragileConstructor::FRAGILE,
    static_cast<int32_t>(*getTransceiverID()),
    0,  // TODO: deprecate
    getChannels());
}

}} // facebook::fboss
