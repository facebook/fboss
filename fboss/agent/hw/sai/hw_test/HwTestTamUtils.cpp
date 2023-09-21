// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestTamUtils.h"

#include "fboss/agent/hw/sai/hw_test/SaiSwitchEnsemble.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/api/TamApi.h"

namespace facebook::fboss {
namespace {
void triggerBcmXgsParityError(HwSwitchEnsemble* ensemble) {
  std::string out;
  auto asic = ensemble->getPlatform()->getAsic()->getAsicType();
  ensemble->runDiagCommand("\n", out);
  if (asic == cfg::AsicType::ASIC_TYPE_TOMAHAWK4 ||
      asic == cfg::AsicType::ASIC_TYPE_TOMAHAWK5) {
    ensemble->runDiagCommand("ser inject pt=L2_ENTRY_SINGLEm\n", out);
    ensemble->runDiagCommand("ser LOG\n", out);
  } else {
    ensemble->runDiagCommand(
        "ser INJECT memory=L2_ENTRY index=10 pipe=pipe_x\n", out);
    ensemble->runDiagCommand("d chg L2_ENTRY 10 1\n", out);
  }
  ensemble->runDiagCommand("quit\n", out);
  std::ignore = out;
}

void triggerBcmJerichoParityError(HwSwitchEnsemble* ensemble) {
  std::string out;
  ensemble->runDiagCommand("\n", out);
  ensemble->runDiagCommand("s CGM_INTERRUPT_MASK_REGISTER -1\n", out);
  ensemble->runDiagCommand("s CGM_ENABLE_DYNAMIC_MEMORY_ACCESS 1\n", out);
  ensemble->runDiagCommand("w CGM_QSPM 0 1 0\n", out);
  ensemble->runDiagCommand(
      "m ECI_GLOBAL_MEM_OPTIONS CPU_BYPASS_ECC_PAR=1\n", out);
  ensemble->runDiagCommand("w CGM_QSPM 0 1 0\n", out);
  ensemble->runDiagCommand(
      "m ECI_GLOBAL_MEM_OPTIONS CPU_BYPASS_ECC_PAR=0\n", out);
  ensemble->runDiagCommand("d raw disable_cache CGM_QSPM 0 1\n", out);
  ensemble->runDiagCommand("quit\n", out);
  std::ignore = out;
}

void triggerCiscoParityError(HwSwitchEnsemble* ensemble) {
  SaiSwitchEnsemble* saiEnsemble = static_cast<SaiSwitchEnsemble*>(ensemble);

  if (SaiSwitchTraits::Attributes::HwEccErrorInitiate::AttributeId()()) {
    SaiSwitchTraits::Attributes::HwEccErrorInitiate initiateError{1};
    auto switchId = static_cast<SwitchSaiId>(saiEnsemble->getSdkSwitchId());
    SaiApiTable::getInstance()->switchApi().setAttribute(
        switchId, initiateError);
  }
}
} // namespace

namespace utility {
void triggerParityError(HwSwitchEnsemble* ensemble) {
  auto asic = ensemble->getPlatform()->getAsic()->getAsicType();
  switch (asic) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
      XLOG(FATAL) << "Unsupported HwAsic: "
                  << ensemble->getPlatform()->getAsic()->getAsicTypeStr();
      break;
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_YUBA:
      triggerCiscoParityError(ensemble);
      break;
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      triggerBcmXgsParityError(ensemble);
      break;
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
      triggerBcmJerichoParityError(ensemble);
      break;
  }
}
} // namespace utility
} // namespace facebook::fboss
