// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace facebook::fboss::loopback_utils {

constexpr std::string_view kModeSystem = "system";
constexpr std::string_view kModeLine = "line";
constexpr std::string_view kActionEnable = "enable";
constexpr std::string_view kActionDisable = "disable";

constexpr int kLoopbackPage = 0x13;
constexpr int kMediaNearLbEnOffset = 181;
constexpr int kMediaFarLbEnOffset = 183;

struct LoopbackCapability {
  bool capSystem{false};
  bool capLine{false};
};

class LoopbackAction : public utils::BaseObjectArgType<std::string> {
 public:
  LoopbackAction() = default;
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ LoopbackAction(std::vector<std::string> v)
      : BaseObjectArgType(v) {
    if (v.empty()) {
      throw FbossError(
          "Incomplete command, expecting '<system|line> <enable|disable>' or 'disable'");
    }
    if (v.size() == 1) {
      if (v[0] == kActionDisable) {
        disableAll_ = true;
        return;
      }
      throw FbossError(
          "Unexpected argument '",
          v[0],
          "', expecting '<system|line> <enable|disable>' or 'disable'");
    }
    if (v.size() != 2) {
      throw FbossError("Expected exactly '<system|line> <enable|disable>'");
    }
    if (v[0] == kModeSystem) {
      mode_ = std::string(kModeSystem);
    } else if (v[0] == kModeLine) {
      mode_ = std::string(kModeLine);
    } else {
      throw FbossError(
          "Unknown mode '", v[0], "', expecting 'system' or 'line'");
    }
    if (v[1] == kActionEnable) {
      enable_ = true;
    } else if (v[1] == kActionDisable) {
      enable_ = false;
    } else {
      throw FbossError("Expected 'enable' or 'disable', got '", v[1], "'");
    }
  }

  bool isDisableAll() const {
    return disableAll_;
  }
  const std::string& mode() const {
    return mode_;
  }
  bool enable() const {
    return enable_;
  }

 private:
  bool disableAll_{false};
  std::string mode_;
  bool enable_{false};
};

// Argument for the unified `set interface <intf> loopback` command:
//   <component> <enable|disable>
//
// The component vocabulary is intentionally shared with `prbs <component>` via
// prbsComponents(), so the two commands can never drift apart:
//   asic | xphy_system | xphy_line | transceiver_system | transceiver_line
class LoopbackComponentAction : public utils::BaseObjectArgType<std::string> {
 public:
  LoopbackComponentAction() = default;
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ LoopbackComponentAction(std::vector<std::string> v);

  phy::PortComponent component() const {
    return component_;
  }
  bool enable() const {
    return enable_;
  }
  const std::string& componentName() const {
    return componentName_;
  }

 private:
  phy::PortComponent component_{phy::PortComponent::GB_LINE};
  bool enable_{false};
  std::string componentName_;
};

// Applies a transceiver loopback action to one port and returns a
// human-readable report (capability check, before/after register state).
// Shared by `set transceiver <port> loopback` and
// `set interface <intf> loopback transceiver_{system,line}`.
std::string setTransceiverLoopbackForPort(
    apache::thrift::Client<FbossCtrl>* agent,
    apache::thrift::Client<QsfpService>* qsfpService,
    const std::string& portName,
    const LoopbackAction& action,
    const std::map<int32_t, PortInfoThrift>& portEntries);

std::map<int32_t, PortInfoThrift> fetchAllPortInfo(
    apache::thrift::Client<FbossCtrl>* agent);

int32_t resolveTransceiverId(
    apache::thrift::Client<FbossCtrl>* agent,
    const std::string& portName,
    const std::map<int32_t, PortInfoThrift>& portEntries);

LoopbackCapability fetchLoopbackCapability(
    apache::thrift::Client<QsfpService>* qsfpService,
    int32_t transceiverId);

uint8_t readOneByte(
    apache::thrift::Client<QsfpService>* qsfpService,
    int32_t transceiverId,
    int page,
    int offset);

std::string formatState(uint8_t mediaNear, uint8_t mediaFar);

} // namespace facebook::fboss::loopback_utils
