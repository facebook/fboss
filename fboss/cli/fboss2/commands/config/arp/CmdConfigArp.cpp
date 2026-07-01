/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/arp/CmdConfigArp.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/String.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {
constexpr std::string_view kAttrTimeout = "timeout";
constexpr std::string_view kAttrAgeInterval = "age-interval";
constexpr std::string_view kAttrMaxProbes = "max-probes";
constexpr std::string_view kAttrStaleInterval = "stale-interval";
constexpr std::string_view kAttrProactive = "proactive";
/* arpRefreshSeconds is defined in switch_config.thrift yet not implemented
constexpr std::string_view kAttrRefresh = "refresh";
*/

// Accepted values for the boolean `proactive` toggle.
constexpr std::string_view kProactiveEnabled = "enabled";
constexpr std::string_view kProactiveDisabled = "disabled";

// Valid ARP/NDP attribute names accepted by `fboss2-dev config arp <attr>`.
// Boolean attrs (kAttrProactive) are parsed in the kAttrProactive branch of
// ArpConfigArgs(); add any new boolean attr there and in applyArpConfig() too.
// Alphabetical order so folly::join produces a stable, sorted error message.
constexpr auto kArpValidAttrs = std::to_array<std::string_view>({
    kAttrAgeInterval,
    kAttrMaxProbes,
    kAttrProactive,
    kAttrStaleInterval,
    kAttrTimeout,
});

std::string applyArpConfig(
    const ArpConfigArgs& args,
    cfg::SwitchConfig& swConfig) {
  if (args.getAttribute() == kAttrProactive) {
    const auto val = args.getBoolValue();
    // NOTE: proactiveArp is declared in switch_config.thrift but is not yet
    // consumed by the agent (no read in ApplyThriftConfig.cpp, no field in
    // switch_state.thrift). Writing it produces a valid config delta but has
    // no runtime effect today; the CLI is wired ahead of the agent support.
    // TODO: when agent support lands, verify HITLESS is still appropriate or
    // change to WARMBOOT if the agent requires a restart to apply the field.
    swConfig.proactiveArp() = val;
    return fmt::format(
        "Set arp {} to {} (stored in config; no runtime effect until agent support lands)",
        kAttrProactive,
        val ? kProactiveEnabled : kProactiveDisabled);
  }

  const auto& attr = args.getAttribute();
  const int32_t val = args.getValue();
  if (attr == kAttrTimeout) {
    swConfig.arpTimeoutSeconds() = val;
  } else if (attr == kAttrAgeInterval) {
    swConfig.arpAgerInterval() = val;
  } else if (attr == kAttrMaxProbes) {
    swConfig.maxNeighborProbes() = val;
  } else if (attr == kAttrStaleInterval) {
    swConfig.staleEntryInterval() = val;
  } else {
    // ArpConfigArgs validates this; defensive guard in case kArpValidAttrs
    // drifts from the dispatch switch here.
    throw std::runtime_error(fmt::format("Unhandled ARP attribute '{}'", attr));
  }
  return fmt::format("Set arp {} to {}", attr, val);
}
} // namespace

ArpConfigArgs::ArpConfigArgs(std::vector<std::string> v) {
  if (v.size() != 2) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <attr> <value>, got {} argument(s). Valid attrs: {}",
            v.size(),
            folly::join(", ", kArpValidAttrs)));
  }

  if (std::find(kArpValidAttrs.begin(), kArpValidAttrs.end(), v[0]) ==
      kArpValidAttrs.end()) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown ARP attribute '{}'. Valid attrs: {}",
            v[0],
            folly::join(", ", kArpValidAttrs)));
  }

  if (v[0] == kAttrProactive) {
    if (v[1] != kProactiveEnabled && v[1] != kProactiveDisabled) {
      throw std::invalid_argument(
          fmt::format(
              "Value for '{}' must be '{}' or '{}', got '{}'",
              v[0],
              kProactiveEnabled,
              kProactiveDisabled,
              v[1]));
    }
    attrKind_ = AttrKind::kBoolean;
    boolValue_ = (v[1] == kProactiveEnabled);
  } else {
    int32_t parsed = 0;
    try {
      parsed = folly::to<int32_t>(v[1]);
    } catch (const folly::ConversionError&) {
      throw std::invalid_argument(
          fmt::format(
              "Value for '{}' must be an integer, got '{}'", v[0], v[1]));
    }
    if (parsed < 0) {
      throw std::invalid_argument(
          fmt::format(
              "Value for '{}' must be non-negative, got {}", v[0], parsed));
    }
    value_ = parsed;
  }

  attribute_ = v[0];
  data_ = std::move(v);
}

CmdConfigArpTraits::RetType CmdConfigArp::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& swConfig = *session.getAgentConfig().sw();

  auto result = applyArpConfig(args, swConfig);

  // The ARP/NDP timer attributes are applied hitlessly by
  // ThriftConfigApplier::updateSwitchSettings (no SAI ChangeProhibited guard).
  // proactiveArp is currently a no-op in the agent, so persisting it is also
  // hitless.
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  return result;
}

void CmdConfigArp::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdConfigArp, CmdConfigArpTraits>::run();

} // namespace facebook::fboss
