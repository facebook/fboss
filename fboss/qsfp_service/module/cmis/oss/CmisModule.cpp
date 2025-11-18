// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/cmis/CmisModule.h"

namespace facebook {
namespace fboss {

/*
 * Always return false for vendor-specific.
 */
bool CmisModule::programAppSelInLowPowerMode() const {
  return false;
}

} // namespace fboss
} // namespace facebook
