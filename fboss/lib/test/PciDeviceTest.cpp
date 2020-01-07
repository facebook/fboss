/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/Singleton.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <stdexcept>

extern "C" {
#include <pciaccess.h>
}

#include "fboss/lib/PciDevice.h"

namespace facebook::fboss {
TEST(PciDevice, InitAnyDevice) {
  uint64_t bar0;
  uint64_t barSize0;
  PciDevice pciDevice(PCI_MATCH_ANY, PCI_MATCH_ANY);
  ASSERT_NO_THROW(pciDevice.open());
  CHECK_EQ(pciDevice.isGood(), true);
  ASSERT_NO_THROW({ bar0 = pciDevice.getMemoryRegionAddress(); });
  ASSERT_NO_THROW({ barSize0 = pciDevice.getMemoryRegionSize(); });
  ASSERT_NO_THROW(pciDevice.close());
}

TEST(PciDevice, DeviceNotFound) {
  PciDevice pciDevice(0x1234, 0xABCD);
  EXPECT_THROW(pciDevice.open(), std::exception);
}

TEST(PciDevice, StaleDevice) {
  PciDevice pciDevice(PCI_MATCH_ANY, PCI_MATCH_ANY);
  pciDevice.open();
  CHECK_EQ(pciDevice.isGood(), true);
  pciDevice.close();
  EXPECT_THROW(
      { auto bar0 = pciDevice.getMemoryRegionAddress(); }, std::exception);
  EXPECT_THROW(
      { auto barSize0 = pciDevice.getMemoryRegionSize(); }, std::exception);
}

TEST(PciDevice, TwoDevices) {
  // Instantiate first device
  PciDevice pciDevice1(PCI_MATCH_ANY, PCI_MATCH_ANY);
  pciDevice1.open();
  CHECK_EQ(pciDevice1.isGood(), true);

  // Instantiate second device
  PciDevice pciDevice2(PCI_MATCH_ANY, PCI_MATCH_ANY);
  pciDevice2.open();
  CHECK_EQ(pciDevice2.isGood(), true);

  // Close first device and access second
  pciDevice1.close();
  ASSERT_NO_THROW({ auto barSize0 = pciDevice2.getMemoryRegionSize(); });

  // Close second device
  ASSERT_NO_THROW(pciDevice2.close());
}

} // namespace facebook::fboss
