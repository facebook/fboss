/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/mdio/Phy.h"

#include <cstdint>
#include <mutex>

#include <folly/Synchronized.h>

namespace facebook {
namespace fboss {

/*
 * Base classes for managing MDIO reads and writes. There are the
 * following primitives:
 *
 * Mdio: base class for logic to actually perform reads/writes. This
 * class should have all platform-specific logic for performing these
 * transactions. There is no locking done at this level.
 *
 * MdioController: Wrapper around an MDIO variant that provides
 * locking.  This should be used to model the number of actual MDIO
 * controllers on the platform and it will make sure we don't try to
 * do concurrent reads using the same controller. It does not provide
 * locking on the level of an MDIO bus, so currently mdio devices on
 * the same bus need to go through the same mdio controller to
 * properly synchronize.
 *
 * MdioDevice: Struct containing an MdioController and a physical
 * address. This should be enough to perform reads/writes to a given
 * chip accessible through Mdio.
 *
 * MdioController and MdioDevice are templated types based on the
 * variant of Mdio being used.
 */

class Mdio {
 public:
  virtual ~Mdio() {}

  // read/write apis.
  virtual phy::Cl45Data readCl45(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr) = 0;

  virtual void writeCl45(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr,
      phy::Cl45Data data) = 0;
};

template <typename IO>
class MdioController {
 public:
  using LockedPtr = typename folly::Synchronized<IO, std::mutex>::LockedPtr;

  template <typename... Args>
  explicit MdioController(Args&&... args)
      : io_(IO(std::forward<Args>(args)...)) {}

  phy::Cl45Data readCl45(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr) {
    return io_.lock()->readCl45(physAddr, devAddr, regAddr);
  }

  void writeCl45(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr,
      phy::Cl45Data data) {
    io_.lock()->writeCl45(physAddr, devAddr, regAddr, data);
  }

  // This can be useful by clients to do multiple MDIO reads/writes
  // w/out releasing the controller. Sample:
  // {
  //   auto io = controller.lock();
  //   io->write(...);
  //   io->read(...);
  // }
  LockedPtr lock() {
    return io_.lock();
  }

 private:
  folly::Synchronized<IO, std::mutex> io_;
};

template <typename IO>
struct MdioDevice {
  // MdioDevice is convenience wrapper for bundling a controller and a
  // physical address together. These two pieces of information are
  // all that should be needed to access MDIO devices. This is a view
  // type of class that is explicitly tied to existence of an
  // MdioController object.
  MdioDevice(MdioController<IO>& controller, phy::PhyAddress address)
      : controller(controller), address(address) {}

  MdioController<IO>& controller;
  const phy::PhyAddress address{0};
};

} // namespace fboss
} // namespace facebook
