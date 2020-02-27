// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include <folly/dynamic.h>

namespace facebook::fboss {

/*
 * This class encapsulates much of the warm boot functionality for an individual
 * unit object. It will store all the files necessary to perform warm boot on a
 * given unit in the passed in warmBootDir. If no directory name is provided,
 * this class will default to a cold boot and perform none of the setup for
 * performing a warm boot on the next startup.
 */
class BcmWarmBootHelper {
 public:
  BcmWarmBootHelper(int unit, std::string warmBootDir);
  ~BcmWarmBootHelper();

  bool canWarmBoot() const {
    return canWarmBoot_;
  }
  void setCanWarmBoot();
  void warmBootRead(uint8_t* buf, int offset, int nbytes);
  void warmBootWrite(const uint8_t* buf, int offset, int nbytes);
  bool storeWarmBootState(const folly::dynamic& switchState);
  bool warmBootStateWritten() const {
    return warmBootStateWritten_;
  }
  folly::dynamic getWarmBootState() const;

  std::string startupSdkDumpFile() const;
  std::string shutdownSdkDumpFile() const;
  std::string warmBootDataPath() const;

 private:
  // Forbidden copy constructor and assignment operator
  BcmWarmBootHelper(BcmWarmBootHelper const&) = delete;
  BcmWarmBootHelper& operator=(BcmWarmBootHelper const&) = delete;

  static int
  warmBootReadCallback(int unitNumber, uint8_t* buf, int offset, int nbytes);
  static int
  warmBootWriteCallback(int unitNumber, uint8_t* buf, int offset, int nbytes);
  std::string warmBootFlag() const;
  std::string forceColdBootOnceFlag() const;
  std::string warmBootSwitchStateFile() const;

  void setupWarmBootFile();
  void setupSdkWarmBoot();

  /*
   * Check to see if we can attempt a warm boot.
   * Returns true if 2 conditions are met
   *
   * 1) User did not create cold_boot_once_unit# file
   * 2) can_warm_boot_unit# file exists, indicating that fboss agent
   * saved warm boot state and shut down successfully.
   * This function also removes the 2 files so that these checks are
   * attempted afresh based on how the agent exits this time and whether
   * or not the user wishes for another cold boot.
   */
  bool checkAndClearWarmBootFlags();

  int unit_{-1};
  std::string warmBootDir_;
  int warmBootFd_{-1};
  bool canWarmBoot_{false};
  bool warmBootStateWritten_{false};
};

} // namespace facebook::fboss
