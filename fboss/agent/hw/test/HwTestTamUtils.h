// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace facebook::fboss {

class HwSwitchEnsemble;
namespace utility {
void triggerParityError(HwSwitchEnsemble* ensemble);
} // namespace utility
} // namespace facebook::fboss
