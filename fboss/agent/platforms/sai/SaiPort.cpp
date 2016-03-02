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
#include "SaiPort.h"
#include "fboss/agent/hw/sai/SaiPortBase.h"

namespace facebook { namespace fboss {

SaiPort::SaiPort(PortID id)
  : id_(id) {
}

void SaiPort::SetPort(SaiPortBase *port) {
  port_ = port;
}

void SaiPort::preDisable(bool temporary) {
}

void SaiPort::postDisable(bool temporary) {
}

void SaiPort::preEnable() {
}

void SaiPort::postEnable() {
}

bool SaiPort::isMediaPresent() {
  return false;
}

void SaiPort::statusIndication(bool enabled, bool link,
                               bool ingress, bool egress,
                               bool discards, bool errors) {
}

}} // facebook::fboss

