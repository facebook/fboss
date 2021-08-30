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

#include <folly/IPAddress.h>
#include <string>

namespace facebook::fboss::utils {

enum class ObjectArgTypeId : uint8_t {
  OBJECT_ARG_TYPE_ID_NONE = 0,
  OBJECT_ARG_TYPE_ID_IPV6_LIST,
  OBJECT_ARG_TYPE_ID_PORT_LIST,
};

const folly::IPAddress getIPFromHost(const std::string& hostname);
const std::string getOobNameFromHost(const std::string& host);
std::vector<std::string> getHostsInSmcTier(const std::string& parentTierName);
std::vector<std::string> getHostsFromFile(const std::string& filename);

void setLogLevel(std::string logLevelStr);

void logUsage(const std::string& cmdName);

} // namespace facebook::fboss::utils
