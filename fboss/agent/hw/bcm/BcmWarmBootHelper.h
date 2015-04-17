// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include <folly/Range.h>

namespace facebook { namespace fboss {


/*
 * This class encapsulates much of the warm boot functionality for an individual
 * unit object. It will store all the files necessary to perform warm boot on a
 * given unit in the passed in warmBootDir. If no directory name is provided,
 * this class will default to a cold boot and perform none of the setup for
 * performing a warm boot on the next startup.
 */
class BcmWarmBootHelper {
public:
  explicit BcmWarmBootHelper(int unit, std::string warmBootDir="");
  ~BcmWarmBootHelper();

  bool canWarmBoot();

  /*
   * Sets a flag that can be read when we next start up that indicates that
   * warm boot is possible. Since warm boot is not currently supported this is
   * always a no-op for now.
   */
  void setCanWarmBoot();

private:
  // Forbidden copy constructor and assignment operator
  BcmWarmBootHelper(BcmWarmBootHelper const &) = delete;
  BcmWarmBootHelper& operator=(BcmWarmBootHelper const &) = delete;

  std::string warmBootFlag() const;
  std::string warmBootDataPath() const;
  std::string forceColdBootOnceFlag() const;

  void setupWarmBootFile();

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

  void warmBootRead(uint8_t* buf, int offset, int nbytes);
  void warmBootWrite(const uint8_t* buf, int offset, int nbytes);

  static int warmBootReadCallback(int unit, uint8_t* buf, int offset,
                                  int nbytes);
  static int warmBootWriteCallback(int unit, uint8_t* buf, int offset,
                                   int nbytes);

  int unit_{-1};
  bool canWarmBoot_{false};
  std::string warmBootDir_;
  int warmBootFd_{-1};
};

}} // facebook::fboss
