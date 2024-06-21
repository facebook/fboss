// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/util/qsfp/QsfpUtilTx.h"
#include <folly/logging/xlog.h>
#include <sysexits.h>

namespace facebook::fboss {

QsfpUtilTx::QsfpUtilTx(
    DirectI2cInfo i2cInfo,
    const std::vector<std::string>& portNames,
    folly::EventBase& evb)
    : bus_(i2cInfo.bus),
      wedgeManager_(i2cInfo.transceiverManager),
      allPortNames_(portNames),
      evb_(evb),
      disableTx_(FLAGS_tx_disable ? true : false) {
  std::string allPortNames;
  std::map<int32_t, TransceiverManagementInterface> moduleTypes;
  std::vector<unsigned int> moduleIds;
  for (auto portName : portNames) {
    allPortNames += portName;
    allPortNames += ", ";

    int moduleId = wedgeManager_->getPortNameToModuleMap().at(portName);
    int oneIndexedModuleId = moduleId + 1;
    moduleIds.push_back(oneIndexedModuleId);
  }

  // Find TransceiverManagementInterface for all these port modules
  moduleTypes = getModuleType(moduleIds);

  XLOG(INFO) << fmt::format(
      "TxDisableTrace: disableTx_ = {:s}, {:s}, FLAGS_tx_disable = {:s}, FLAGS_tx_enable = {:s}",
      allPortNames,
      disableTx_ ? "disabled" : "enabled",
      FLAGS_tx_disable ? "disabled" : "enabled",
      FLAGS_tx_enable ? "enabled" : "disabled");

  for (auto portName : portNames) {
    int moduleId = wedgeManager_->getPortNameToModuleMap().at(portName);
    int oneIndexedModuleId = moduleId + 1;
    if (moduleTypes.find(oneIndexedModuleId) == moduleTypes.end()) {
      XLOG(ERR) << fmt::format(
          "Port {:s} does not a valid module type", portName);
      continue;
    }
    auto moduleType = moduleTypes[oneIndexedModuleId];

    if (moduleType == TransceiverManagementInterface::SFF) {
      sffPortNames_.push_back(portName);
    } else if (moduleType == TransceiverManagementInterface::CMIS) {
      cmisPortNames_.push_back(portName);
    } else {
      XLOG(ERR) << fmt::format(
          "Port {:s} has unknown managament interface type", portName);
    }
  }
}

int QsfpUtilTx::setTxDisable() {
  return FLAGS_direct_i2c ? setTxDisableDirect() : setTxDisableViaService();
}

/*
 * This function directly disables the optics lane TX which brings down the
 * module. The TX Disable will cause LOS at the link partner and Remote Fault at
 * this end.
 */
int QsfpUtilTx::setTxDisableDirect() {
  int retVal = EX_OK;
  if (!sffPortNames_.empty()) {
    if (!setSffTxDisableDirect(sffPortNames_)) {
      retVal |= EX_SOFTWARE;
    }
  }
  if (!cmisPortNames_.empty()) {
    if (!setCmisTxDisableDirect(cmisPortNames_)) {
      retVal |= EX_SOFTWARE;
    }
  }
  return retVal;
}

/*
 * This function directly disables the SFF optics lane TX which brings down the
 * module. The TX Disable will cause LOS at the link partner and Remote Fault at
 * this end.
 */
bool QsfpUtilTx::setSffTxDisableDirect(
    const std::vector<std::string>& sffPortNames) {
  for (auto portName : sffPortNames) {
    int moduleId = wedgeManager_->getPortNameToModuleMap().at(portName);
    int oneIndexedModuleId = moduleId + 1;
    XLOG(INFO) << fmt::format(
        "TxDisableTrace: SFF {:d}: directly {:s} TX on {:s} channels",
        oneIndexedModuleId,
        disableTx_ ? "disabled" : "enabled",
        (FLAGS_channel >= 1 && FLAGS_channel <= QsfpUtilTx::maxSffChannels)
            ? "some"
            : "all");

    try {
      // For SFF module, the lower page 0 reg 86 controls TX_DISABLE for 4 lanes
      const int offset = 86;
      const int length = 1;
      uint8_t buf;

      bus_->moduleRead(
          oneIndexedModuleId,
          {TransceiverAccessParameter::ADDR_QSFP, offset, length},
          &buf);
      setChannelDisable(TransceiverManagementInterface::SFF, buf);
      bus_->moduleWrite(
          oneIndexedModuleId,
          {TransceiverAccessParameter::ADDR_QSFP, offset, length},
          &buf);
    } catch (const I2cError&) {
      XLOG(ERR) << fmt::format(
          "TxDisableTrace: QSFP {:d}: unwritable or write error",
          oneIndexedModuleId);
      return false;
    }
  }
  return true;
}

/*
 * This function directly disables the CMIS optics lane TX which brings down the
 * module. The TX Disable will cause LOS at the link partner and Remote Fault at
 * this end.
 */
bool QsfpUtilTx::setCmisTxDisableDirect(
    const std::vector<std::string>& cmisPortNames) {
  for (auto portName : cmisPortNames) {
    int moduleId = wedgeManager_->getPortNameToModuleMap().at(portName);
    int oneIndexedModuleId = moduleId + 1;
    XLOG(INFO) << fmt::format(
        "TxDisableTrace: CMIS {:d}: directly {:s} TX on {:s} channels",
        oneIndexedModuleId,
        disableTx_ ? "disabled" : "enabled",
        (FLAGS_channel >= 1 && FLAGS_channel <= QsfpUtilTx::maxCmisChannels)
            ? "some"
            : "all");

    try {
      const int offset = 127;
      const int length = 1;
      uint8_t savedPage;

      // Save current page
      bus_->moduleRead(
          oneIndexedModuleId,
          {TransceiverAccessParameter::ADDR_QSFP, offset, length},
          &savedPage);

      // For CMIS module, the page 0x10 reg 130 controls TX_DISABLE for 8 lanes
      uint8_t moduleControlPage = 0x10;
      const int cmisTxDisableReg = 130;
      uint8_t buf;

      bus_->moduleWrite(
          oneIndexedModuleId,
          {TransceiverAccessParameter::ADDR_QSFP, offset, length},
          &moduleControlPage);
      bus_->moduleRead(
          oneIndexedModuleId,
          {TransceiverAccessParameter::ADDR_QSFP, cmisTxDisableReg, length},
          &buf);
      setChannelDisable(TransceiverManagementInterface::CMIS, buf);
      bus_->moduleWrite(
          oneIndexedModuleId,
          {TransceiverAccessParameter::ADDR_QSFP, cmisTxDisableReg, length},
          &buf);

      // Restore current page. Need a delay here, otherwise certain modules
      // will fail to turn on the lasers. Sleep override
      usleep(20 * 1000);
      bus_->moduleWrite(
          oneIndexedModuleId,
          {TransceiverAccessParameter::ADDR_QSFP, offset, length},
          &savedPage);
    } catch (const I2cError&) {
      XLOG(ERR) << fmt::format(
          "TxDisableTrace: QSFP {:d}: read/write error", oneIndexedModuleId);
      return false;
    }
  }
  return true;
}

/*
 * This function indirectly disables the optics lane TX via qsfp_service
 * which brings down the module. The TX Disable will cause LOS at the link
 * partner and Remote Fault at this end.
 */
int QsfpUtilTx::setTxDisableViaService() {
  int retVal = EX_OK;

  // Release the bus access for QSFP service
  bus_->close();

  if (!setTxDisableViaServiceHelper(allPortNames_)) {
    retVal |= EX_SOFTWARE;
  }

  return retVal;
}

/*
 * This function indirectly disables the CMIS or SFF optics lane TX via
 * qsfp_service which brings down the module. The TX Disable will cause LOS at
 * the link partner and Remote Fault at this end.
 */
bool QsfpUtilTx::setTxDisableViaServiceHelper(
    const std::vector<std::string>& portNames) {
  std::vector<phy::TxRxEnableRequest> txDisableRequests;

  for (auto portName : portNames) {
    XLOG(INFO) << fmt::format(
        "TxDisableTrace: {:s}: indirectly {:s} TX on {:s} channels",
        portName,
        disableTx_ ? "disabled" : "enabled",
        (FLAGS_channel != -1) ? "some" : "all");

    phy::TxRxEnableRequest txRequest;
    txRequest.portName() = portName;
    txRequest.component() = phy::PortComponent::TRANSCEIVER_LINE;
    txRequest.direction() = phy::Direction::TRANSMIT;
    txRequest.enable() = !disableTx_;
    if (FLAGS_channel != -1) {
      txRequest.laneMask() = 1 << (FLAGS_channel - 1);
    }

    txDisableRequests.push_back(txRequest);
  }

  auto client = getQsfpClient(evb_);
  std::vector<phy::TxRxEnableResponse> txDisableResponses;

  try {
    client->sync_setInterfaceTxRx(txDisableResponses, txDisableRequests);
    for (const auto& response : txDisableResponses) {
      XLOG(INFO) << fmt::format(
          "Tx Dis/Ena operation on port {:s} is {:s}",
          response.portName().value(),
          (response.success().value() ? "successful" : "failed"));
    }
  } catch (const std::exception& ex) {
    XLOG(ERR) << fmt::format(
        "Tx Dis/Ena operation through qsfp_service failed: {:s}", ex.what());
    return false;
  }
  return true;
}

/*
 * Set proper mask to disable selected channels
 */
void QsfpUtilTx::setChannelDisable(
    const TransceiverManagementInterface moduleType,
    uint8_t& data) {
  const uint8_t maxChannels =
      (moduleType == TransceiverManagementInterface::CMIS)
      ? QsfpUtilTx::maxCmisChannels
      : QsfpUtilTx::maxSffChannels;
  const uint8_t channelMask =
      (moduleType == TransceiverManagementInterface::CMIS) ? 0xFF : 0xF;

  if (FLAGS_channel >= 1 && FLAGS_channel <= maxChannels) {
    // Disable/enable a particular channel in module
    if (disableTx_) {
      data |= (1 << (FLAGS_channel - 1));
    } else {
      data &= ~(1 << (FLAGS_channel - 1));
    }
  } else {
    // Disable/enable all channels
    data = disableTx_ ? channelMask : 0x0;
  }
}

} // namespace facebook::fboss
