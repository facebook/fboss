/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/fixmyfboss/CheckEnvironment.h"

#include <stdexcept>

#include <folly/String.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/platform_checks/LocalHost.h"
#include "fboss/platform/platform_checks/PlatformName.h"

namespace facebook::fboss::platform::fixmyfboss {

using namespace platform_checks;

namespace {

constexpr std::chrono::seconds kConnectTimeout{30};

std::shared_ptr<const Host> connect(
    const std::string& hostname,
    RemoteHost::Transport transport) {
  auto host = std::make_shared<RemoteHost>(hostname, transport);
  XLOG(INFO) << "Connecting to " << hostname;
  auto result = host->run("true", kConnectTimeout);
  if (!result.ok()) {
    throw std::runtime_error(
        "Cannot connect to " + hostname + ": " +
        folly::trimWhitespace(result.standardErr).str());
  }
  return host;
}

void connectBmc(const ConnectOptions& options, CheckEnvironment& env) {
  if (options.noBmc) {
    env.bmcUnavailableReason = "BMC checks disabled with --no-bmc";
    return;
  }
  if (options.bmcHostname) {
    // Explicitly requested, so failing to connect is fatal.
    env.bmc = connect(*options.bmcHostname, options.transport);
    return;
  }
  if (!options.hostname) {
    env.bmcUnavailableReason =
        "No BMC to check; pass --hostname or --bmc-hostname";
    return;
  }
  // Reported by the BMC Reachable check rather than aborting the run, so the
  // x86 is still checked. Switches without a BMC need --no-bmc.
  auto oobHostname = deriveOobHostname(*options.hostname);
  try {
    env.bmc = connect(oobHostname, options.transport);
  } catch (const std::exception& ex) {
    XLOG(WARN) << ex.what();
    env.bmcConnectError = ex.what();
    env.bmcUnavailableReason =
        "Cannot connect to BMC " + oobHostname + "; see BMC Reachable";
  }
}

} // namespace

std::string deriveOobHostname(const std::string& hostname) {
  auto dot = hostname.find('.');
  if (dot == std::string::npos) {
    return hostname + "-oob";
  }
  return hostname.substr(0, dot) + "-oob" + hostname.substr(dot);
}

CheckEnvironment createEnvironment(const ConnectOptions& options) {
  CheckEnvironment env;
  env.x86 = options.hostname ? connect(*options.hostname, options.transport)
                             : std::make_shared<LocalHost>();
  auto platformName = getPlatformName(*env.x86);
  if (!platformName) {
    throw std::runtime_error(
        "Failed to determine platform name of " + env.x86->name());
  }
  env.platformName = *platformName;
  connectBmc(options, env);
  return env;
}

} // namespace facebook::fboss::platform::fixmyfboss
