/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>

#include <gflags/gflags.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

extern int mdio_read(int phy, int dev, int reg);
extern void mdio_write(int phy, int dev, int reg, int data);

static void usage(const char* cmd) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "\t%s read PHY DEVICE REGISTER\n", cmd);
  fprintf(stderr, "\t%s write PHY DEVICE REGISTER DATA\n", cmd);
}

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);

  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }

  bool doWrite{false};

  int phyAddr;
  int devAddr;
  int regAddr;
  int data;

  if (strcmp(argv[1], "read") == 0) {
    if (argc != 5) {
      usage(argv[0]);
      return 1;
    }
  } else if (strcmp(argv[1], "write") == 0) {
    if (argc != 6) {
      usage(argv[0]);
      return 1;
    }
    doWrite = true;
  }

  auto checkParameter = [](int value, int low, int high, const char *what) {
    if (value < low || value > high) {
      fprintf(stderr, "Invalid %s %#x\n", what, value);
      exit(2);
    }
  };

  phyAddr = strtoul(argv[2], nullptr, 0);
  checkParameter(phyAddr, 0, 31, "PHY");

  devAddr = strtoul(argv[3], nullptr, 0);
  checkParameter(devAddr, 0, 31, "device");

  regAddr = strtoul(argv[4], nullptr, 0);
  checkParameter(regAddr, 0, 255 * 255, "register");

  if (doWrite) {
    data = strtoul(argv[5], nullptr, 0);
    checkParameter(data, 0, 255 * 255, "data");
  }

  try {
    if (doWrite) {
      mdio_write(phyAddr, devAddr, regAddr, data);
    } else {
      printf("%#x\n", mdio_read(phyAddr, devAddr, regAddr));
    }
  } catch (const std::exception& ex) {
    fprintf(stderr, "error: failed to %s MDIO device: %s\n",
            (doWrite) ? "write" : "read",
            ex.what());
    return 1;
  }

  return 0;
}
