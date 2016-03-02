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

#include "fboss/agent/PlatformPort.h"

namespace facebook { namespace fboss {

class SaiPortBase;

class SaiPlatformPort : public PlatformPort {
public:
  SaiPlatformPort() {}
  SaiPlatformPort(SaiPlatformPort&&) = default;
  SaiPlatformPort &operator=(SaiPlatformPort&&) = default;

  /*
   * SetPort() should be called once during port initialization.
   */
  virtual void SetPort(SaiPortBase *port) = 0;
  virtual SaiPortBase *GetPort() const = 0;

private:
  // Forbidden copy constructor and assignment operator
  SaiPlatformPort(SaiPlatformPort const &) = delete;
  SaiPlatformPort &operator=(SaiPlatformPort const &) = delete;
};

}} // facebook::fboss
