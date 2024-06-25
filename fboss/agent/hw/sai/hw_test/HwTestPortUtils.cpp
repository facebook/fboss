/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestPortUtils.h"

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/diag/DiagShell.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortUtils.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/platforms/sai/SaiPlatformPort.h"

#include "folly/testing/TestUtil.h"

#include <algorithm>

namespace facebook::fboss::utility {

void setPortLoopbackMode(
    const HwSwitch* hwSwitch,
    PortID portId,
    cfg::PortLoopbackMode lbMode) {
  // Use concurrent indices to make this thread safe
  const auto& portMapping = static_cast<const SaiSwitch*>(hwSwitch)
                                ->concurrentIndices()
                                .portSaiId2PortInfo;
  // ConcurrentHashMap deletes copy constructor for its iterators, making it
  // impossible to use a std::find_if here. So rollout a ugly hand written loop
  auto portItr = portMapping.begin();
  for (; portItr != portMapping.end(); ++portItr) {
    if (portItr->second.portID == portId) {
      break;
    }
  }
  CHECK(portItr != portMapping.end());
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  SaiPortTraits::Attributes::PortLoopbackMode portLoopbackMode{
      utility::getSaiPortLoopbackMode(lbMode)};
  auto& portApi = SaiApiTable::getInstance()->portApi();
  portApi.setAttribute(portItr->first, portLoopbackMode);
#else
  SaiPortTraits::Attributes::InternalLoopbackMode internalLbMode{
      utility::getSaiPortInternalLoopbackMode(lbMode)};
  auto& portApi = SaiApiTable::getInstance()->portApi();
  portApi.setAttribute(portItr->first, internalLbMode);
#endif
}

void setCreditWatchdogAndPortTx(const HwSwitch* hw, PortID port, bool enable) {
  setPortTx(hw, port, enable);
  if (hw->getPlatform()->getAsic()->getSwitchType() == cfg::SwitchType::VOQ) {
    enableCreditWatchdog(hw, enable);
  }
}

void enableCreditWatchdog(const HwSwitch* hw, bool enable) {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  auto switchID = static_cast<const SaiSwitch*>(hw)->getSaiSwitchId();
  SaiApiTable::getInstance()->switchApi().setAttribute(
      switchID, SaiSwitchTraits::Attributes::CreditWd{enable});
#endif
}

void setPortTx(const HwSwitch* hw, PortID port, bool enable) {
  auto portHandle = static_cast<const SaiSwitch*>(hw)
                        ->managerTable()
                        ->portManager()
                        .getPortHandle(port);

  portHandle->port->setOptionalAttribute(
      SaiPortTraits::Attributes::PktTxEnable{enable});
}
void enableTransceiverProgramming(bool enable) {
  FLAGS_skip_transceiver_programming = !enable;
}

void injectFecError(
    std::vector<int> hwPorts,
    const HwSwitch* hw,
    bool injectCorrectable) {
  if (hw->getPlatform()->getAsic()->getAsicVendor() !=
      HwAsic::AsicVendor::ASIC_VENDOR_BCM) {
    throw FbossError("FEC error injection only supported on BCM ASICs");
  }
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto diagShell = std::make_unique<DiagShell>(saiSwitch);
  auto diagCmdServer =
      std::make_unique<DiagCmdServer>(saiSwitch, diagShell.get());

  constexpr auto kBaseFecErrorInjStr = R"(
  cint_reset();
  bcm_port_phy_fec_error_mask_t injection;
  uint8 error_control_map = 0xFF;
  int unit = 0;
  )";
  std::string kFecErrorInjStr = kBaseFecErrorInjStr;
  if (injectCorrectable) {
    kFecErrorInjStr = folly::sformat(
        "\n{}\n injection.error_mask_bit_63_32=0x1;", kFecErrorInjStr);
  } else {
    kFecErrorInjStr = folly::sformat(
        "\n{}\n injection.error_mask_bit_63_32=0xFFFFFFFF;", kFecErrorInjStr);
  }
  for (auto hwPortID : hwPorts) {
    int portNum = hwPortID;
    kFecErrorInjStr = folly::sformat(
        "\n{}\n print bcm_port_phy_fec_error_inject_set(unit, {}, error_control_map, injection);",
        kFecErrorInjStr,
        portNum);
  }
  folly::test::TemporaryFile file;
  folly::writeFull(
      file.fd(), kFecErrorInjStr.c_str(), std::strlen(kFecErrorInjStr.c_str()));

  XLOG(INFO) << "CINT = " << kFecErrorInjStr;
  ClientInformation clientInfo;
  clientInfo.username() = "hw_test";
  clientInfo.hostname() = "hw_test";
  auto out = diagCmdServer->diagCmd(
      std::make_unique<fbstring>(
          folly::sformat("cint {}\n", file.path().c_str())),
      std::make_unique<ClientInformation>(clientInfo));
  XLOG(INFO) << "OUTPUT = " << out;
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("quit\n"),
      std::make_unique<ClientInformation>(clientInfo));
}

} // namespace facebook::fboss::utility
