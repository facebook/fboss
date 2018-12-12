// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include <folly/dynamic.h>
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
  BcmWarmBootHelper() {}
  virtual ~BcmWarmBootHelper() {}

  virtual bool canWarmBoot() const = 0;
  /*
   * Sets a flag that can be read when we next start up that indicates that
   * warm boot is possible. Since warm boot is not currently supported this is
   * always a no-op for now.
   */
  virtual void setCanWarmBoot() = 0;
  static int
  warmBootReadCallback(int unit, uint8_t* buf, int offset, int nbytes);
  static int
  warmBootWriteCallback(int unit, uint8_t* buf, int offset, int nbytes);

  virtual void warmBootRead(uint8_t* buf, int offset, int nbytes) = 0;
  virtual void warmBootWrite(const uint8_t* buf, int offset, int nbytes) = 0;

  virtual bool storeWarmBootState(const folly::dynamic& switchState) = 0;
  virtual folly::dynamic getWarmBootState() const = 0;

  virtual std::string startupSdkDumpFile() const = 0;
  virtual std::string shutdownSdkDumpFile() const = 0;

 private:
  // Forbidden copy constructor and assignment operator
  BcmWarmBootHelper(BcmWarmBootHelper const&) = delete;
  BcmWarmBootHelper& operator=(BcmWarmBootHelper const&) = delete;
};

class DiscBackedBcmWarmBootHelper: public BcmWarmBootHelper {

 public:
  DiscBackedBcmWarmBootHelper(int unit, std::string warmBootDir);
  ~DiscBackedBcmWarmBootHelper() override;

  bool canWarmBoot() const override {
    return canWarmBoot_;
  }
  void setCanWarmBoot() override;
  void warmBootRead(uint8_t* buf, int offset, int nbytes) override;
  void warmBootWrite(const uint8_t* buf, int offset, int nbytes) override;
  bool storeWarmBootState(const folly::dynamic& switchState) override;
  folly::dynamic getWarmBootState() const override;

  std::string startupSdkDumpFile() const override;
  std::string shutdownSdkDumpFile() const override;

 private:
  // Forbidden copy constructor and assignment operator
  DiscBackedBcmWarmBootHelper(DiscBackedBcmWarmBootHelper const&) = delete;
  DiscBackedBcmWarmBootHelper& operator=(DiscBackedBcmWarmBootHelper const&) =
      delete;

  std::string warmBootFlag() const;
  std::string warmBootDataPath() const;
  std::string forceColdBootOnceFlag() const;
  std::string warmBootSwitchStateFile() const;

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

  int unit_{-1};
  std::string warmBootDir_;
  int warmBootFd_{-1};
  bool canWarmBoot_{false};
};

}} // facebook::fboss
