// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspIOBus.h"
#include <folly/logging/xlog.h>

namespace facebook::fboss {

/*
 * This function does the i2c read on a given module.
 */
void BspIOBus::moduleRead(
    unsigned int module,
    const TransceiverAccessParameter& param,
    uint8_t* buf) {
  XLOG(DBG5) << fmt::format("BspTrace: moduleRead() for module={:d}", module);
  CHECK(module >= 1 && module <= systemContainer_->getNumTransceivers());
  systemContainer_->tcvrRead(module, param, buf);
}

/*
 * This function does the i2c write on a given module.
 */
void BspIOBus::moduleWrite(
    unsigned int module,
    const TransceiverAccessParameter& param,
    const uint8_t* data) {
  XLOG(DBG5) << fmt::format("BspTrace: moduleWrite() for module={:d}", module);
  CHECK(module >= 1 && module <= systemContainer_->getNumTransceivers());
  systemContainer_->tcvrWrite(module, param, data);
}

// Check if a QSFP is present using QSFP controller
bool BspIOBus::isPresent(unsigned int module) {
  CHECK(module >= 1 && module <= systemContainer_->getNumTransceivers());

  XLOG(DBG5) << fmt::format(
      "BspTrace: isPresent() detecting presence of qsfp={:d}", module);

  return systemContainer_->isTcvrPresent(module);
}

/*
 * This function consolidates the counters from all controllers and returns the
 * vector of the i2c stats
 */
std::vector<I2cControllerStats> BspIOBus::getI2cControllerStats() {
  XLOG(DBG1) << "BspTrace: getI2cControllerStats()";
  std::vector<I2cControllerStats> i2cControllerCurrentStats;

  for (auto module = 1; module <= systemContainer_->getNumTransceivers();
       ++module) {
    i2cControllerCurrentStats.push_back(
        systemContainer_->getI2cControllerStats(module));
  }

  return i2cControllerCurrentStats;
}

/*
 * This function checks for all the modules in input map and fills in if the
 * module is present or not
 */
void BspIOBus::scanPresence(std::map<int32_t, ModulePresence>& presences) {
  XLOG(DBG1) << "BspTrace: scanPresence()";

  for (auto& presence : presences) {
    // Calling platform specific bus class to determine if the module is
    // present. The platform knows which ports are valid
    if (isPresent(presence.first + 1)) {
      presence.second = ModulePresence::PRESENT;
    } else {
      presence.second = ModulePresence::ABSENT;
    }
  }
}

void BspIOBus::initTransceivers() {
  XLOG(DBG1) << "BspTrace: initTransceivers()";

  systemContainer_->initAllTransceivers();
}

/*
 * Start the I2c read/write time profiling
 */
void BspIOBus::i2cTimeProfilingStart(unsigned int module) const {
  CHECK(module >= 1 && module <= systemContainer_->getNumTransceivers());
  systemContainer_->i2cTimeProfilingStart(module);
}

/*
 * Stop the I2c read/write time profiling
 */
void BspIOBus::i2cTimeProfilingEnd(unsigned int module) const {
  CHECK(module >= 1 && module <= systemContainer_->getNumTransceivers());
  systemContainer_->i2cTimeProfilingEnd(module);
}

/*
 * Return the result of i2c timing profiling in the format of
 * <readTimeMsec, writeTimeMsec>
 */
std::pair<uint64_t, uint64_t> BspIOBus::getI2cTimeProfileMsec(
    unsigned int module) const {
  CHECK(module >= 1 && module <= systemContainer_->getNumTransceivers());
  return systemContainer_->getI2cTimeProfileMsec(module);
}

} // namespace facebook::fboss
