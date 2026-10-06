/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/platform_checks/RemoteHost.h"

#include <stdexcept>

namespace facebook::fboss::platform::platform_checks {

namespace {

constexpr auto kSushReason = "fixmyfboss";

// Switches are reprovisioned often, so their host keys churn.
const std::vector<std::string> kSshOptions{
    "-o",
    "BatchMode=yes",
    "-o",
    "ConnectTimeout=10",
    "-o",
    "StrictHostKeyChecking=no",
    "-o",
    "UserKnownHostsFile=/dev/null",
    "-o",
    "LogLevel=ERROR",
};

std::vector<std::string> concat(
    std::vector<std::string> head,
    const std::vector<std::string>& tail) {
  head.insert(head.end(), tail.begin(), tail.end());
  return head;
}

} // namespace

RemoteHost::Transport RemoteHost::detectTransport() {
  return runLocalCommand({"which", "sush2"}, std::chrono::seconds(5)).ok()
      ? Transport::SUSH2
      : Transport::SSH;
}

RemoteHost::RemoteHost(
    std::string hostname,
    Transport transport,
    std::string user)
    : hostname_(std::move(hostname)),
      transport_(transport),
      user_(std::move(user)) {}

CommandResult RemoteHost::run(
    const std::string& cmd,
    std::chrono::seconds timeout) const {
  return runLocalCommand(execArgv(cmd), timeout);
}

void RemoteHost::copyTo(
    const std::filesystem::path& localPath,
    const std::filesystem::path& hostPath,
    std::chrono::seconds timeout) const {
  auto result = runLocalCommand(copyArgv(localPath, hostPath), timeout);
  if (!result.ok()) {
    throw std::runtime_error(
        "Copying " + localPath.string() + " to " + hostname_ + ":" +
        hostPath.string() + " failed (exit " + std::to_string(result.exitCode) +
        "): " + result.standardErr);
  }
}

std::vector<std::string> RemoteHost::execArgv(const std::string& cmd) const {
  switch (transport_) {
    case Transport::SSH:
      return concat(concat({"ssh"}, kSshOptions), {destination(), cmd});
    case Transport::SUSH2:
      return {"sush2", "-q", "--reason", kSushReason, destination(), "--", cmd};
  }
  throw std::logic_error("Unknown transport");
}

std::vector<std::string> RemoteHost::copyArgv(
    const std::filesystem::path& localPath,
    const std::filesystem::path& hostPath) const {
  auto target = destination() + ":" + hostPath.string();
  switch (transport_) {
    case Transport::SSH:
      return concat(
          concat({"scp", "-r"}, kSshOptions), {localPath.string(), target});
    case Transport::SUSH2:
      return {
          "suscp2", "-r", "--reason", kSushReason, localPath.string(), target};
  }
  throw std::logic_error("Unknown transport");
}

std::string RemoteHost::destination() const {
  return user_ + "@" + hostname_;
}

} // namespace facebook::fboss::platform::platform_checks
