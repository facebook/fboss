// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestTamUtils.h"

#include "fboss/agent/hw/sai/hw_test/SaiSwitchEnsemble.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/api/TamApi.h"

namespace facebook::fboss {
namespace {
void triggerSaiBcmParityError(HwSwitchEnsemble* ensemble) {
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
} // namespace
namespace utility {
void triggerParityError(HwSwitchEnsemble* ensemble) {
  auto asic = ensemble->getPlatform()->getAsic()->getAsicType();
  if (asic != cfg::AsicType::ASIC_TYPE_EBRO) {
    triggerSaiBcmParityError(ensemble);
    return;
  }
  SaiSwitchEnsemble* saiEnsemble = static_cast<SaiSwitchEnsemble*>(ensemble);
  if (SaiSwitchTraits::Attributes::HwEccErrorInitiate::AttributeId()()) {
    SaiSwitchTraits::Attributes::HwEccErrorInitiate initiateError{1};
    auto switchId = static_cast<SwitchSaiId>(saiEnsemble->getSdkSwitchId());
    SaiApiTable::getInstance()->switchApi().setAttribute(
        switchId, initiateError);
  }
}
} // namespace utility
} // namespace facebook::fboss
