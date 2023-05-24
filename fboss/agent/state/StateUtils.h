/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <string>

#include "fboss/agent/types.h"

namespace facebook::fboss {

namespace util {

/**
 * Utility functions for InterfaceID <-> ifName (on host)
 */
std::string createTunIntfName(InterfaceID ifID);
bool isTunIntfName(const std::string& ifName);
InterfaceID getIDFromTunIntfName(const std::string& ifName);

template <typename MswitchMapT>
auto getFirstMap(const std::shared_ptr<MswitchMapT>& map) {
  return map->size() ? map->cbegin()->second : nullptr;
}

} // namespace util

} // namespace facebook::fboss
