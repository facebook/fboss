// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/diag/DiagShell.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "folly/testing/TestUtil.h"

namespace facebook {
namespace fboss {
namespace utility {

void HwTestThriftHandler::injectFecError(
    std::unique_ptr<std::vector<int>> hwPorts,
    bool injectCorrectable) {
  if (hwSwitch_->getPlatform()->getAsic()->getAsicVendor() !=
      HwAsic::AsicVendor::ASIC_VENDOR_BCM) {
    throw FbossError("FEC error injection only supported on BCM ASICs");
  }
  auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch_);
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
  for (auto hwPortID : *hwPorts) {
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

} // namespace utility
} // namespace fboss
} // namespace facebook
