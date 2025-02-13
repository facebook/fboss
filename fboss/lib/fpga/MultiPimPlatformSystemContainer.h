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

#include "fboss/lib/fpga/MultiPimPlatformPimContainer.h"
#include "fboss/lib/if/gen-cpp2/pim_state_types.h"

#include <map>
#include <memory>

namespace facebook::fboss {

class FpgaDevice;

/*
 * Multi-pim Platform System Container base class.
 * As FPGA is the main interface that we use in FBOSS multi-pim platforms to
 * manage features like LED, QSFP, External Phy, this base class maintain the
 * FpgaDevice.
 */
class MultiPimPlatformSystemContainer {
 public:
  explicit MultiPimPlatformSystemContainer(
      std::unique_ptr<FpgaDevice> fpgaDevice);
  virtual ~MultiPimPlatformSystemContainer();

  FpgaDevice* getFpgaDevice() const {
    return fpgaDevice_.get();
  }

  virtual uint32_t getPimOffset(int pim) = 0;

  virtual uint8_t getPimStartNum() const = 0;

  /**
   * Map the FPGA into the process, and init the HW.
   *
   * If forceReset is set, will put all HW into reset and then out of reset.
   */
  virtual void initHW(bool forceReset = false) = 0;

  virtual MultiPimPlatformPimContainer* getPimContainer(int pim) const;

  virtual MultiPimPlatformPimContainer::PimType getPimType(int pim) = 0;

  size_t getNumPims() const {
    return pims_.size();
  }

  bool isValidPimID(int pim) const {
    return (pims_.find(pim) != pims_.end());
  }

  std::map<int, PimState> getPimStates() const;

 protected:
  void setPimContainer(
      int pim,
      std::unique_ptr<MultiPimPlatformPimContainer> pimContainer);

 private:
  // Forbidden copy constructor and assignment operator
  MultiPimPlatformSystemContainer(MultiPimPlatformSystemContainer const&) =
      delete;
  MultiPimPlatformSystemContainer& operator=(
      MultiPimPlatformSystemContainer const&) = delete;

  std::unique_ptr<FpgaDevice> fpgaDevice_;
  std::map<int, std::unique_ptr<MultiPimPlatformPimContainer>> pims_;
};
} // namespace facebook::fboss
