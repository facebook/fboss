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

#include <map>

#include "fboss/agent/Platform.h"

extern "C" {
#include "saitypes.h"
}

namespace facebook { namespace fboss {

class SaiPlatformPort;

class SaiPlatformBase : public Platform {
public:
  typedef std::map<sai_object_id_t, SaiPlatformPort *> InitPortMap;

  SaiPlatformBase() {}

  /*
   * initPorts() will be called during port initialization.
   *
   * The SaiPlatformBase should return a map of SAI port ID to SaiPlatformPort
   * objects.  The SaiPlatformBase object will retain ownership of all the
   * SaiPlatformPort objects, and must ensure that they remain valid for as
   * long as the SaiSwitch exists.
   */
  virtual InitPortMap initPorts() = 0;

private:
  // Forbidden copy constructor and assignment operator
  SaiPlatformBase(SaiPlatformBase const &) = delete;
  SaiPlatformBase &operator=(SaiPlatformBase const &) = delete;
};

}} // facebook::fboss
