// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss {
namespace utility {
void HwTestThriftHandler::triggerParityError() {
  std::string out;
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  auto asic = bcmSwitch->getPlatform()->getAsic()->getAsicType();
  bcmSwitch->printDiagCmd("\n");
  switch (asic) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_AGERA3:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK6:
    case cfg::AsicType::ASIC_TYPE_CHENAB:
      XLOG(FATAL) << "Unsupported HwAsic: "
                  << bcmSwitch->getPlatform()->getAsic()->getAsicTypeStr();
      break;
    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3: {
      bcmSwitch->printDiagCmd(
          "ser INJECT memory=L2_ENTRY index=10 pipe=pipe_x\n");
      bcmSwitch->printDiagCmd("d chg L2_ENTRY 10 1\n");
    } break;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4: {
      bcmSwitch->printDiagCmd("ser inject pt=L2_ENTRY_SINGLEm\n");
      bcmSwitch->printDiagCmd("ser LOG\n");
    } break;
  }
  bcmSwitch->printDiagCmd("quit\n");
  std::ignore = out;
}
} // namespace utility
} // namespace facebook::fboss
