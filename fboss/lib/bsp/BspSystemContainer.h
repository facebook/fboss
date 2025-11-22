// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <limits>
#include <memory>
#include <unordered_map>

#include "fboss/lib/bsp/BspLedContainer.h"
#include "fboss/lib/bsp/BspPimContainer.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/lib/fpga/FpgaDevice.h"
#include "fboss/lib/phy/PhyManager.h"

namespace facebook {
namespace fboss {

class BspSystemContainer {
 public:
  // Platforms that need to create a FPGA device (for example to read the PIM
  // type) should use the ctor with FpgaDevice arg and then call
  // initializePimContainers() in the derived class ctor. The base class
  // (BspSystemContainer) ctor will mmap the fpga which is required to do
  // read/write.
  explicit BspSystemContainer(std::unique_ptr<FpgaDevice> fpgaDevice);

  // An expectation of this constructor is to initialize the Pim Containers
  explicit BspSystemContainer(std::unique_ptr<BspPlatformMapping> bspMapping);

  BspPlatformMapping* getBspPlatformMapping() const {
    return bspMapping_.get();
  }

  FpgaDevice* getFpgaDevice() const {
    return fpgaDevice_.get();
  }

  void createBspLedContainers();

  const BspPimContainer* getPimContainerFromPimID(int pimID) const;
  const BspPimContainer* getPimContainerFromTcvrID(int tcvrID) const;
  BspDeviceMdioController* getMdioController(int pimID, int controllerID) const;
  std::map<uint32_t, LedIO*> getLedController(int tcvrID) const;
  int getNumTransceivers() const;
  int getPimIDFromTcvrID(int tcvrID) const;
  int getNumPims() const;

  bool isTcvrPresent(int tcvrID) const;
  void initAllTransceivers() const;
  void clearAllTransceiverReset() const;
  void initTransceiver(int tcvrID) const;
  void holdTransceiverReset(int tcvrID) const;
  void releaseTransceiverReset(int tcvrID) const;
  const I2cControllerStats getI2cControllerStats(int tcvrID) const;

  void tcvrRead(
      unsigned int tcvrID,
      const TransceiverAccessParameter& param,
      uint8_t* buf) const;
  void tcvrWrite(
      unsigned int tcvrID,
      const TransceiverAccessParameter& param,
      const uint8_t* buf) const;

  virtual bool isPimPresent(int pimID) const {
    return pimContainers_.find(pimID) != pimContainers_.end();
  }

  virtual uint32_t getPimOffset(int /* pim */) const {
    // Platforms that need this, should implement this function.
    CHECK(false);
  }

  virtual uint8_t getPimStartNum() const {
    if (pimContainers_.empty()) {
      // If no PIMs configured, default to 1 for BSP platforms
      return 1;
    }

    uint8_t minPimID = std::numeric_limits<uint8_t>::max();
    for (const auto& [pimID, _] : pimContainers_) {
      if (pimID < minPimID) {
        minPimID = pimID;
      }
    }
    return minPimID;
  }

  virtual void initHW(bool forceReset = false) {}

  virtual MultiPimPlatformPimContainer::PimType getPimType(int pim) {
    // Platforms with PIMs should implement this function.
    CHECK(false);
  }

  void setPhyManager(PhyManager* phyMgr) {
    phyMgr_ = phyMgr;
  }

  phy::ExternalPhy* getExternalPhyObj(phy::PhyIDInfo info) {
    if (phyMgr_->getNumOfSlot() <= 0) {
      // If number of slot is not set that means the SandiaPhyManager is not
      // initialized so return NULL from here
      return nullptr;
    }
    return phyMgr_->getExternalPhy(phyMgr_->getGlobalXphyID(info));
  }

  void i2cTimeProfilingStart(unsigned int tcvrID) const;
  void i2cTimeProfilingEnd(unsigned int tcvrID) const;
  std::pair<uint64_t, uint64_t> getI2cTimeProfileMsec(
      unsigned int tcvrID) const;

  virtual ~BspSystemContainer() {}

 protected:
  // Initialize the PIM Containers. Must be called during
  // construction of the BspSystemContainer class if a BSP
  // mapping is passed. The reason for this is that any derived
  // class can assume the PIM containers to be initialized
  // once the BspSystemContainer is constructed.
  void initializePimContainers();
  std::unique_ptr<BspPlatformMapping> bspMapping_;

 private:
  std::unordered_map<int, std::unique_ptr<BspPimContainer>> pimContainers_;
  std::unique_ptr<FpgaDevice> fpgaDevice_;
  PhyManager* phyMgr_;
};

} // namespace fboss
} // namespace facebook
