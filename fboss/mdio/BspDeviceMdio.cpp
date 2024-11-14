/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/mdio/BspDeviceMdio.h"
#include <fcntl.h>
#include <folly/Format.h>
#include <folly/Range.h>
#include <folly/logging/xlog.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstdint>
#include "fboss/mdio/MdioError.h"

// TODO(rajank):
// Remove this temporary definition and include mdio_ioctl.h
namespace {
using mdio_access_req_t = struct mdio_access_req_t {
  uint32_t addr;
  uint32_t reg;
  uint32_t value;
};

#define MDIO_ACCESS_REGRD _IOR('m', 1, mdio_access_req_t)
#define MDIO_ACCESS_REGWR _IOW('m', 2, mdio_access_req_t)

} // namespace

namespace facebook::fboss {

BspDeviceMdio::BspDeviceMdio(
    int pim,
    int controller,
    const std::string& devName)

    : devName_(devName), pim_(pim), controller_(controller) {
  // Open the Char device interface for this Phy chip
  fd_ = open(devName.c_str(), O_RDWR);

  if (fd_ < 0) {
    // Crash the program because the callers are not handling this exception
    throw MdioError(fmt::format(
        "MdioDevIo() failed to open {}, retVal = {:d}, errno = {}",
        devName,
        fd_,
        folly::errnoStr(errno)));
  }
}

BspDeviceMdio::~BspDeviceMdio() {
  close(fd_);
}

void BspDeviceMdio::init(bool forceReset) {
  Mdio::init(forceReset);
  XLOG(DBG1) << fmt::format(
      "put MDIO controller out of reset pim={:d} controller={:d}",
      pim_,
      controller_);
  const auto usDelay = kDelayAfterMdioInit;
  /* sleep override */ usleep(usDelay);
}

phy::Cl45Data BspDeviceMdio::cl45Operation(
    unsigned int ioctlVal,
    phy::PhyAddress physAddr,
    phy::Cl45DeviceAddress devAddr,
    phy::Cl45RegisterAddress regAddr,
    phy::Cl45Data data) {
  struct mdio_access_req_t req;
  int err;

  request_++;

  req.addr = physAddr;
  // This assumes upper 16 bits are DevType. We might have to make that a mask
  // an argument if other BSPs need it to be different.
  req.reg = (devAddr << 16) | regAddr;
  req.value = (ioctlVal == MDIO_ACCESS_REGWR) ? data : 0;

  // Perform MDIO clause 45 read/write operation
  // (read: MDIO_ACCESS_REGRD, write: MDIO_ACCESS_REGWR)
  err = ioctl(fd_, ioctlVal, &req, sizeof(req));
  if (err) {
    throw MdioError(fmt::format(
        "cl45Operation() failed to {} to {}, errno = {}",
        (ioctlVal == MDIO_ACCESS_REGWR ? "write" : "read"),
        devName_,
        folly::errnoStr(errno)));
  }

  return req.value;
}

phy::Cl45Data BspDeviceMdio::readCl45(
    phy::PhyAddress physAddr,
    phy::Cl45DeviceAddress devAddr,
    phy::Cl45RegisterAddress regAddr) {
  XLOG(DBG5) << "BspDeviceMdio::readCl45 PIM " << pim_ << " Controller "
             << controller_ << " PhysAddr " << (int)physAddr << " Dev "
             << (int)devAddr << " Reg 0x" << std::hex << regAddr;
  phy::Cl45Data val =
      cl45Operation(MDIO_ACCESS_REGRD, physAddr, devAddr, regAddr, 0);
  XLOG(DBG5) << "Read val: 0x" << std::hex << val;
  return val;
}

void BspDeviceMdio::writeCl45(
    phy::PhyAddress physAddr,
    phy::Cl45DeviceAddress devAddr,
    phy::Cl45RegisterAddress regAddr,
    phy::Cl45Data data) {
  XLOG(DBG5) << "BspDeviceMdio::writeCl45 PIM " << pim_ << " Controller "
             << controller_ << " PhysAddr " << (int)physAddr << " Dev "
             << (int)devAddr << " Reg 0x" << std::hex << regAddr;
  cl45Operation(MDIO_ACCESS_REGWR, physAddr, devAddr, regAddr, data);
}

} // namespace facebook::fboss
