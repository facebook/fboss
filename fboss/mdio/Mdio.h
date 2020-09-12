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

#include <folly/Conv.h>
#include "fboss/mdio/Phy.h"
#include "folly/File.h"

#include <cstdint>
#include <mutex>

#include <folly/Synchronized.h>

namespace {
constexpr auto kMdioLockFilePath = "/var/lock/mdio";
}

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
  explicit MdioController(int id, Args&&... args)
      : rawIO_(IO(std::forward<Args>(args)...)),
        io_(rawIO_),
        lockFile_(std::make_shared<folly::File>(
            folly::to<std::string>(kMdioLockFilePath, id),
            O_RDWR | O_CREAT,
            0666)) {}

  // Delete the copy constructor. If we try to copy this object while io_ is locked
  // then the process will immediately deadlock.
  explicit MdioController(MdioController &) = delete;

  phy::Cl45Data readCl45Unlocked(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr) {
    return rawIO_.readCl45(physAddr, devAddr, regAddr);
  }

  void writeCl45Unlocked(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr,
      phy::Cl45Data data) {
    rawIO_.writeCl45(physAddr, devAddr, regAddr, data);
  }

    phy::Cl45Data readCl45(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr) {
    auto locked = fully_lock();
    return locked->readCl45(physAddr, devAddr, regAddr);
  }

  void writeCl45(
      phy::PhyAddress physAddr,
      phy::Cl45DeviceAddress devAddr,
      phy::Cl45RegisterAddress regAddr,
      phy::Cl45Data data) {
    auto locked = fully_lock();
    locked->writeCl45(physAddr, devAddr, regAddr, data);
  }

  // This can be useful by clients to do multiple MDIO reads/writes
  // w/out releasing the controller. Sample:
  // {
  //   auto io = controller.fully_lock();
  //   io->write(...);
  //   io->read(...);
  // }

  // Functions for synchronizing multiple processes' access to
  // MDIO. The calling thread must hold a LockedPtr lock for
  // inter-thread exclusion before attempting to acquire or release
  // an inter-process lock.

  struct ProcLock {
    ProcLock(std::shared_ptr<folly::File> lockFile) : lockFile_(lockFile) {
      lockFile_->lock();
    }

    // Clear the lockFile when moving so that we don't unlock twice
    explicit ProcLock(ProcLock&& old): lockFile_(std::move(old.lockFile_)) {
      old.lockFile_ = nullptr;
    }
    // Delete copy ctor since it doesn't really make sense to have two locks on
    // the same mutex.
    explicit ProcLock(ProcLock&) = delete;

    ~ProcLock() {
      if (lockFile_.get() != nullptr) {
        lockFile_->unlock();
      }
    }

   private:
    std::shared_ptr<folly::File> lockFile_;
  };

  class FullyLockedMdio {
   public:
    FullyLockedMdio(LockedPtr&& threadLock, std::shared_ptr<folly::File> lockFile_)
        : locked_(std::move(threadLock)), procLock_(lockFile_) {}

    FullyLockedMdio(FullyLockedMdio&& old): locked_(std::move(old.locked_)), procLock_(std::move(old.procLock_)) {}
    // Delete copy ctor since we'd just deadlock if we copy the LockedPtr.
    FullyLockedMdio(FullyLockedMdio& old) = delete;

    LockedPtr& operator->() {
      return locked_;
    }

   private:
    LockedPtr locked_;
    ProcLock procLock_;
  };

  FullyLockedMdio fully_lock() {
    auto threadLock = io_.lock();
    return FullyLockedMdio(std::move(threadLock), lockFile_);
  }

  // TODO (ccpowers): remove this as soon as we've stopped using the accton xphy code
  // Same as fully_lock, but using 'new' so that we can delegate ownership of locks
  // to C code that needs them (i.e the Accton xphy commands)
  FullyLockedMdio* fully_lock_new() {
    auto threadLock = io_.lock();
    return new FullyLockedMdio(std::move(threadLock), lockFile_);
  }

 private:
  IO rawIO_;
  folly::Synchronized<IO, std::mutex> io_;
  std::shared_ptr<folly::File> lockFile_;
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
