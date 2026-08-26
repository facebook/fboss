// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
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
