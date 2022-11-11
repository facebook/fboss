/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/PciSystem.h"
#ifndef IS_OSS
#include <pciaccess.h>
#endif

#include <folly/Exception.h>
#include <folly/Singleton.h>

namespace {
folly::Singleton<facebook::fboss::PciSystem> _pciSystem;
}

namespace facebook::fboss {
PciSystem::PciSystem() {
#ifndef IS_OSS
  int32_t retVal;

  retVal = pci_system_init();
  if (retVal != 0) {
    folly::throwSystemErrorExplicit(retVal, "Could not init PCI system");
  }
#endif
}

PciSystem::~PciSystem() {
#ifndef IS_OSS
  pci_system_cleanup();
#endif
}

std::shared_ptr<PciSystem> PciSystem::getInstance() {
  return _pciSystem.try_get();
}

} // namespace facebook::fboss
