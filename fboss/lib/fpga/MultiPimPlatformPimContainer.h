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
#include <vector>
#include "fboss/lib/if/gen-cpp2/pim_state_types.h"

namespace facebook::fboss {
/*
 * Multi-pim Platform Pim Container base class.
 * As each pim has its own fpga, this class can provide functions to access
 * hardware like LED, QSFP, External Phy and etc. on such PIM.
 */
class MultiPimPlatformPimContainer {
 public:
  enum PimType {
    MINIPACK_16Q,
    MINIPACK_16O,
    YAMP_16Q,
    FUJI_16Q,
    FUJI_16O,
    ELBERT_16Q,
    ELBERT_8DD,
  };
  static std::string getPimTypeStr(PimType pimType);
  static PimType getPimTypeFromStr(const std::string& pimTypeStr);

  MultiPimPlatformPimContainer();
  virtual ~MultiPimPlatformPimContainer();

  virtual bool isPimPresent() const = 0;

  virtual void initHW(bool forceReset = false) = 0;

  virtual PimState getPimState() const {
    return PimState();
  }

 private:
  // Forbidden copy constructor and assignment operator
  MultiPimPlatformPimContainer(MultiPimPlatformPimContainer const&) = delete;
  MultiPimPlatformPimContainer& operator=(MultiPimPlatformPimContainer const&) =
      delete;

  // TODO(joseph5wu) We can manage LED/QSFP fpga classes here
};
} // namespace facebook::fboss
