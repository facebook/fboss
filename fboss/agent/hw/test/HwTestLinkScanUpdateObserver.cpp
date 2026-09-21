// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestLinkScanUpdateObserver.h"

namespace facebook::fboss {

HwTestLinkScanUpdateObserver::HwTestLinkScanUpdateObserver(
    HwSwitchEnsemble* ensemble)
    : ensemble_(ensemble) {
  ensemble_->addHwEventObserver(this);
}

HwTestLinkScanUpdateObserver::~HwTestLinkScanUpdateObserver() {
  ensemble_->removeHwEventObserver(this);
}

} // namespace facebook::fboss
