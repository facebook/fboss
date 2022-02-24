// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/util/QsfpUtilTx.h"
#include <folly/logging/xlog.h>
#include <sysexits.h>

namespace facebook::fboss {

QsfpUtilTx::QsfpUtilTx(
    TransceiverI2CApi* bus,
    const std::vector<unsigned int>& ports,
    folly::EventBase& evb)
    : bus_(bus),
      ports_(ports),
      evb_(evb),
      disableTx_(FLAGS_tx_disable ? true : false) {
  XLOG(INFO) << fmt::format(
      "TxDisableTrace: disableTx_ = {:s}, FLAGS_tx_disable = {:s}, FLAGS_tx_enable = {:s}",
      disableTx_ ? "disabled" : "enabled",
      FLAGS_tx_disable ? "disabled" : "enabled",
      FLAGS_tx_enable ? "enabled" : "disabled");
}

int QsfpUtilTx::setTxDisable() {
  return FLAGS_direct_i2c ? setTxDisableDirect() : setTxDisableViaService();
}

/*
 * This function directly disables the optics lane TX which brings down the
 * port. The TX Disable will cause LOS at the link partner and Remote Fault at
 * this end.
 */
int QsfpUtilTx::setTxDisableDirect() {
  int retVal = EX_OK;

  for (unsigned int port : ports_) {
    XLOG(INFO) << fmt::format(
        "TxDisableTrace: QSFP {:d}: directly {:s} TX on {:s} channels",
        port,
        disableTx_ ? "disabled" : "enabled",
        (FLAGS_channel >= 1 && FLAGS_channel <= QsfpUtilTx::maxCmisChannels)
            ? "some"
            : "all");

    // Get module type CMIS or SFF
    auto moduleType = getModuleType(bus_, port);

    if (moduleType == TransceiverManagementInterface::SFF) {
      if (!setSffTxDisableDirect(port)) {
        retVal = EX_SOFTWARE;
      }
    } else if (moduleType == TransceiverManagementInterface::CMIS) {
      if (!setCmisTxDisableDirect(port)) {
        retVal = EX_SOFTWARE;
      }
    } else {
      XLOG(ERR) << fmt::format(
          "TxDisableTrace: QSFP {:d}: Unrecognized transceiver management interface",
          port);
      retVal = EX_SOFTWARE;
    }
  }

  return retVal;
}

/*
 * This function directly disables the SFF optics lane TX which brings down the
 * port. The TX Disable will cause LOS at the link partner and Remote Fault at
 * this end.
 */
bool QsfpUtilTx::setSffTxDisableDirect(unsigned int port) {
  try {
    // For SFF module, the lower page 0 reg 86 controls TX_DISABLE for 4 lanes
    const int offset = 86;
    const int length = 1;
    uint8_t buf;

    bus_->moduleRead(
        port, {TransceiverI2CApi::ADDR_QSFP, offset, length}, &buf);
    setChannelDisable(TransceiverManagementInterface::SFF, buf);
    bus_->moduleWrite(
        port, {TransceiverI2CApi::ADDR_QSFP, offset, length}, &buf);
  } catch (const I2cError& ex) {
    XLOG(ERR) << fmt::format(
        "TxDisableTrace: QSFP {:d}: unwritable or write error", port);
    return false;
  }

  return true;
}

/*
 * This function directly disables the CMIS optics lane TX which brings down the
 * port. The TX Disable will cause LOS at the link partner and Remote Fault at
 * this end.
 */
bool QsfpUtilTx::setCmisTxDisableDirect(unsigned int port) {
  try {
    const int offset = 127;
    const int length = 1;
    uint8_t savedPage;

    // Save current page
    bus_->moduleRead(
        port, {TransceiverI2CApi::ADDR_QSFP, offset, length}, &savedPage);

    // For CMIS module, the page 0x10 reg 130 controls TX_DISABLE for 8 lanes
    uint8_t moduleControlPage = 0x10;
    const int cmisTxDisableReg = 130;
    uint8_t buf;

    bus_->moduleWrite(
        port,
        {TransceiverI2CApi::ADDR_QSFP, offset, length},
        &moduleControlPage);
    bus_->moduleRead(
        port, {TransceiverI2CApi::ADDR_QSFP, cmisTxDisableReg, length}, &buf);
    setChannelDisable(TransceiverManagementInterface::CMIS, buf);
    bus_->moduleWrite(
        port, {TransceiverI2CApi::ADDR_QSFP, cmisTxDisableReg, length}, &buf);

    // Restore current page. Need a delay here, otherwise certain modules
    // will fail to turn on the lasers. Sleep override
    usleep(20 * 1000);
    bus_->moduleWrite(
        port, {TransceiverI2CApi::ADDR_QSFP, offset, length}, &savedPage);
  } catch (const I2cError& ex) {
    XLOG(ERR) << fmt::format(
        "TxDisableTrace: QSFP {:d}: read/write error", port);
    return false;
  }

  return true;
}

/*
 * This function indirectly disables the optics lane TX via qsfp_service
 * which brings down the port. The TX Disable will cause LOS at the link partner
 * and Remote Fault at this end.
 */
int QsfpUtilTx::setTxDisableViaService() {
  // Release the bus access for QSFP service
  bus_->close();
  std::map<int32_t, TransceiverManagementInterface> moduleTypes =
      getModuleTypeViaService(ports_, evb_);

  if (moduleTypes.empty()) {
    XLOG(ERR) << "TxDisableTrace: Indirect read error in getting module type";
    return EX_SOFTWARE;
  }

  std::vector<int32_t> sffPortIdx;
  std::vector<int32_t> cmisPortIdx;

  for (auto [port, moduleType] : moduleTypes) {
    XLOG(INFO) << fmt::format(
        "TxDisableTrace: QSFP {:d}: indirectly {:s} TX on {:s} channels",
        port + 1,
        disableTx_ ? "disabled" : "enabled",
        (FLAGS_channel >= 1 && FLAGS_channel <= QsfpUtilTx::maxCmisChannels)
            ? "some"
            : "all");

    if (moduleType == TransceiverManagementInterface::SFF) {
      sffPortIdx.push_back(port);
    } else if (moduleType == TransceiverManagementInterface::CMIS) {
      cmisPortIdx.push_back(port);
    } else {
      XLOG(ERR) << fmt::format(
          "TxDisableTrace: QSFP {:d}: Unrecognized transceiver management interface",
          port);
      return EX_SOFTWARE;
    }
  }

  if (sffPortIdx.empty() && cmisPortIdx.empty()) {
    XLOG(ERR)
        << "TxDisableTrace: Did not detect any transceivers with expected interface";
    return EX_SOFTWARE;
  }

  if (!sffPortIdx.empty() && !setSffTxDisableViaService(sffPortIdx)) {
    return EX_SOFTWARE;
  }

  if (!cmisPortIdx.empty() && !setCmisTxDisableViaService(cmisPortIdx)) {
    return EX_SOFTWARE;
  }

  return EX_OK;
}

/*
 * This function indirectly disables the SFF optics lane TX via qsfp_service
 * which brings down the port. The TX Disable will cause LOS at the link partner
 * and Remote Fault at this end.
 */
bool QsfpUtilTx::setSffTxDisableViaService(const std::vector<int32_t>& ports) {
  // For SFF module, the page 0 reg 86 controls TX_DISABLE for 4 lanes
  const int offset = 86;
  const int length = 1;
  const int page = 0;
  std::map<int32_t, ReadResponse> readResp =
      doReadRegViaService(ports, offset, length, page, evb_);

  if (readResp.empty()) {
    XLOG(ERR) << "TxDisableTrace: indirect read error";
    return false;
  }

  for (const auto& response : readResp) {
    auto buf = *(response.second.data_ref()->data());
    setChannelDisable(TransceiverManagementInterface::SFF, buf);
    std::vector<int32_t> writePort{response.first};

    if (!doWriteRegViaService(writePort, offset, page, buf, evb_)) {
      XLOG(ERR) << fmt::format(
          "TxDisableTrace: QSFP {:d}: indirect write error", response.first);
      return false;
    }
  }

  XLOG(INFO) << fmt::format(
      "TxDisableTrace: successfully {:s} SFF Tx indirectly",
      disableTx_ ? "disabled" : "enabled");
  return true;
}

/*
 * This function indirectly disables the CMIS optics lane TX via qsfp_service
 * which brings down the port. The TX Disable will cause LOS at the link partner
 * and Remote Fault at this end.
 */
bool QsfpUtilTx::setCmisTxDisableViaService(const std::vector<int32_t>& ports) {
  // For CMIS module, page 0x10 of reg 130 controls TX_DISABLE for 8 lanes
  const int length = 1;
  const uint8_t moduleControlPage = 0x10;
  const int cmisTxDisableReg = 130;
  std::map<int32_t, ReadResponse> readResp = doReadRegViaService(
      ports, cmisTxDisableReg, length, moduleControlPage, evb_);

  if (readResp.empty()) {
    XLOG(ERR) << "TxDisableTrace: indirect read error";
    return false;
  }

  for (const auto& response : readResp) {
    auto buf = *(response.second.data_ref()->data());
    setChannelDisable(TransceiverManagementInterface::CMIS, buf);
    std::vector<int32_t> writePort{response.first};

    if (!doWriteRegViaService(
            writePort, cmisTxDisableReg, moduleControlPage, buf, evb_)) {
      XLOG(ERR) << fmt::format(
          "TxDisableTrace: QSFP {:d}: indirect write error", response.first);
      return false;
    }
  }

  XLOG(INFO) << fmt::format(
      "TxDisableTrace: successfully {:s} CMIS Tx indirectly",
      disableTx_ ? "disabled" : "enabled");
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
