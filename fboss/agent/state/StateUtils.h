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

namespace facebook {
namespace fboss {
namespace util {
/*
 * Compatibility between cpp2 and cpp enum names.
 *
 * cpp2: `EnumType::EnumValue`
 * cpp: `EnumValue`
 *
 * TODO: Temporary until next fboss agent release. After that we can take
 * away this logic.
 */
std::string getCpp2EnumName(const std::string& enumValue);

/**
* Utility functions for InterfaceID <-> ifName (on host)
*/
std::string createTunIntfName(InterfaceID ifID);
bool isTunIntfName(const std::string& ifName);
InterfaceID getIDFromTunIntfName(const std::string& ifName);

} // namespace util
} // namespace fboss
} // namespace facebook
