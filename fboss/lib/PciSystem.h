/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <cstdint>
#include <memory>

struct pci_device;

namespace facebook::fboss {
/**
 * Singleton class for accessing PCI subsystem access
 */
class PciSystem {
 public:
  PciSystem();
  ~PciSystem();
  static std::shared_ptr<PciSystem> getInstance();
};

} // namespace facebook::fboss
