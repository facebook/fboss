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

#include "fboss/agent/hw/sai/SaiPlatformPort.h"

namespace facebook { namespace fboss {

class SaiPortBase;

class SaiPort : public SaiPlatformPort {
public:
  explicit SaiPort(PortID id);

  virtual PortID getPortID() const {
    return id_;
  }

  void setPort(SaiPortBase *port) override;
  SaiPortBase *getPort() const override {
    return port_;
  }

  void preDisable(bool temporary) override;
  void postDisable(bool temporary) override;
  void preEnable() override;
  void postEnable() override;
  bool isMediaPresent() override;
  void linkStatusChanged(bool up, bool adminUp) override;
  void statusIndication(bool enabled, bool link,
                        bool ingress, bool egress,
                        bool discards, bool errors) override;

private:
  // Forbidden copy constructor and assignment operator
  SaiPort(SaiPort const &) = delete;
  SaiPort &operator=(SaiPort const &) = delete;

  PortID id_ {0};
  SaiPortBase *port_ {nullptr};
};

}} // facebook::fboss
