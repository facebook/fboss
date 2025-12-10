// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/diag/DiagShell.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"
/*
#include "fboss/agent/hw/sai/switch/SaiHandler.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
*/

namespace facebook::fboss {
namespace {

void triggerBcmXgsParityError(const HwSwitch* hw) {
  auto asic = hw->getPlatform()->getAsic()->getAsicType();
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto diagShell = std::make_unique<DiagShell>(saiSwitch);
  auto diagCmdServer =
      std::make_unique<DiagCmdServer>(saiSwitch, diagShell.get());
  ClientInformation clientInfo;
  clientInfo.username() = "hw_test";
  clientInfo.hostname() = "hw_test";

  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("\n"),
      std::make_unique<ClientInformation>(clientInfo));

  if (asic == cfg::AsicType::ASIC_TYPE_TOMAHAWK4 ||
      asic == cfg::AsicType::ASIC_TYPE_TOMAHAWK5 ||
      asic == cfg::AsicType::ASIC_TYPE_TOMAHAWK6) {
    diagCmdServer->diagCmd(
        std::make_unique<fbstring>("ser inject pt=L2_ENTRY_SINGLEm\n"),
        std::make_unique<ClientInformation>(clientInfo));
    diagCmdServer->diagCmd(
        std::make_unique<fbstring>("ser LOG\n"),
        std::make_unique<ClientInformation>(clientInfo));
  } else {
    diagCmdServer->diagCmd(
        std::make_unique<fbstring>(
            "ser INJECT memory=L2_ENTRY index=10 pipe=pipe_x\n"),
        std::make_unique<ClientInformation>(clientInfo));
    diagCmdServer->diagCmd(
        std::make_unique<fbstring>("d chg L2_ENTRY 10 1\n"),
        std::make_unique<ClientInformation>(clientInfo));
  }
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("quit\n"),
      std::make_unique<ClientInformation>(clientInfo));
}

void triggerBcmJericho2ParityError(const HwSwitch* hw) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto diagShell = std::make_unique<DiagShell>(saiSwitch);
  auto diagCmdServer =
      std::make_unique<DiagCmdServer>(saiSwitch, diagShell.get());
  ClientInformation clientInfo;
  clientInfo.username() = "hw_test";
  clientInfo.hostname() = "hw_test";

  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("\n"),
      std::make_unique<ClientInformation>(clientInfo));

  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("s CGM_INTERRUPT_MASK_REGISTER -1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("s CGM_ENABLE_DYNAMIC_MEMORY_ACCESS 1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("w CGM_QSPM 0 1 0\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>(
          "m ECI_GLOBAL_MEM_OPTIONS CPU_BYPASS_ECC_PAR=1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("w CGM_QSPM 0 1 1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>(
          "m ECI_GLOBAL_MEM_OPTIONS CPU_BYPASS_ECC_PAR=0\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("d raw disable_cache CGM_QSPM 0 1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("quit\n"),
      std::make_unique<ClientInformation>(clientInfo));
}

void triggerBcmJericho3ParityError(const HwSwitch* hw) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto diagShell = std::make_unique<DiagShell>(saiSwitch);
  auto diagCmdServer =
      std::make_unique<DiagCmdServer>(saiSwitch, diagShell.get());
  ClientInformation clientInfo;
  clientInfo.username() = "hw_test";
  clientInfo.hostname() = "hw_test";

  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("\n"),
      std::make_unique<ClientInformation>(clientInfo));

  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("s CGM_INTERRUPT_MASK_REGISTER -1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>(
          "w CGM_QSPM 0 1 system_port_0=0 system_port_1=0 system_port_2=0 system_port_3=0\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>(
          "m ECI_GLOBAL_MEM_OPTIONS CPU_BYPASS_ECC_PAR=1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>(
          "w CGM_QSPM 0 1 system_port_0=0 system_port_1=0 system_port_2=0 system_port_3=1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>(
          "m ECI_GLOBAL_MEM_OPTIONS CPU_BYPASS_ECC_PAR=0\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("d disable_cache CGM_QSPM 0 1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("quit\n"),
      std::make_unique<ClientInformation>(clientInfo));
}

void triggerBcmRamonParityError(const HwSwitch* hw) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto diagShell = std::make_unique<DiagShell>(saiSwitch);
  auto diagCmdServer =
      std::make_unique<DiagCmdServer>(saiSwitch, diagShell.get());
  ClientInformation clientInfo;
  clientInfo.username() = "hw_test";
  clientInfo.hostname() = "hw_test";

  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("\n"),
      std::make_unique<ClientInformation>(clientInfo));

  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("debug bcm intr +\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("ser eccindication off\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("s RTP_INTERRUPT_MASK_REGISTER -1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("s RTP_ENABLE_DYNAMIC_MEMORY_ACCESS 1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("w RTP_TOTSF 0 1 0\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>(
          "m ECI_GLOBAL_MEM_OPTIONS CPU_BYPASS_ECC_PAR=1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("w RTP_TOTSF 0 1 1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>(
          "m ECI_GLOBAL_MEM_OPTIONS CPU_BYPASS_ECC_PAR=0\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("d raw disable_cache RTP_TOTSF 0 1\n"),
      std::make_unique<ClientInformation>(clientInfo));
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("quit\n"),
      std::make_unique<ClientInformation>(clientInfo));
}

void triggerCiscoParityError(const HwSwitch* hw) {
  if (SaiSwitchTraits::Attributes::HwEccErrorInitiate::AttributeId()()) {
    SaiSwitchTraits::Attributes::HwEccErrorInitiate initiateError{1};
    auto saiSwitch = static_cast<const SaiSwitch*>(hw);
    auto switchId = saiSwitch->getSaiSwitchId();
    SaiApiTable::getInstance()->switchApi().setAttribute(
        switchId, initiateError);
  }
}

void triggerChenabParityError(const HwSwitch* hw) {
  XLOG(INFO) << "Triggering Correctable Parity Error";
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  if (SaiSwitchTraits::Attributes::TriggerSimulatedEccCorrectableError::
          AttributeId()()) {
    SaiSwitchTraits::Attributes::TriggerSimulatedEccCorrectableError
        initiateError{true};
    auto switchId = static_cast<SwitchSaiId>(saiSwitch->getSaiSwitchId());
    SaiApiTable::getInstance()->switchApi().setAttribute(
        switchId, initiateError);
  }
}

} // namespace

namespace utility {
void HwTestThriftHandler::triggerParityError() {
  auto asic = hwSwitch_->getPlatform()->getAsic()->getAsicType();

  switch (asic) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_AGERA3:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      triggerBcmRamonParityError(hwSwitch_);
      break;
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_YUBA:
      triggerCiscoParityError(hwSwitch_);
      break;
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK6:
      triggerBcmXgsParityError(hwSwitch_);
      break;
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
      triggerBcmJericho2ParityError(hwSwitch_);
      break;
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
      triggerBcmJericho3ParityError(hwSwitch_);
      break;
    case cfg::AsicType::ASIC_TYPE_CHENAB:
      triggerChenabParityError(hwSwitch_);
      break;
  }
}
} // namespace utility
} // namespace facebook::fboss
