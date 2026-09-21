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

#include <string>

namespace facebook::fboss {
/**
 * Class for managing pci device access.
 */
class PciAccess {
 public:
  explicit PciAccess(std::string devicePath);
  ~PciAccess();

  void enableMemSpaceAccess();

 private:
  std::string devicePath_;
};

} // namespace facebook::fboss
