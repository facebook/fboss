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

} // namespace

CheckEnvironment createEnvironment(const ConnectOptions& options) {
  std::shared_ptr<const Host> x86 = options.hostname
      ? connect(*options.hostname, options.transport)
      : std::make_shared<LocalHost>();
  auto platformName = getPlatformName(*x86);
  if (!platformName) {
    throw std::runtime_error(
        "Failed to determine platform name of " + x86->name());
  }
  return CheckEnvironment{.platformName = *platformName, .x86 = std::move(x86)};
}

} // namespace facebook::fboss::platform::fixmyfboss
