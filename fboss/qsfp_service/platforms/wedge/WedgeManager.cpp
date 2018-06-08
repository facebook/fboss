#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

#include <folly/gen/Base.h>

#include <folly/logging/xlog.h>
#include "fboss/lib/usb/UsbError.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeQsfp.h"
#include "fboss/qsfp_service/sff/QsfpModule.h"

namespace facebook { namespace fboss {

WedgeManager::WedgeManager() {}

void WedgeManager::initTransceiverMap() {
  // If we can't get access to the USB devices, don't bother to
  // create the QSFP objects;  this is likely to be a permanent
  // error.
  try {
    wedgeI2CBusLock_ = std::make_unique<WedgeI2CBusLock>(getI2CBus());
  } catch (const LibusbError& ex) {
    XLOG(ERR) << "failed to initialize USB to I2C interface";
    return;
  }

  // Wedge port 0 is the CPU port, so the first port associated with
  // a QSFP+ is port 1.  We start the transceiver IDs with 0, though.
  for (int idx = 0; idx < getNumQsfpModules(); idx++) {
    auto qsfpImpl = std::make_unique<WedgeQsfp>(idx, wedgeI2CBusLock_.get());
    auto qsfp = std::make_unique<QsfpModule>(
        std::move(qsfpImpl), numPortsPerTransceiver());
    qsfp->refresh();
    transceivers_.push_back(move(qsfp));
    XLOG(INFO) << "making QSFP for " << idx;
  }
}

void WedgeManager::getTransceiversInfo(std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  XLOG(INFO) << "Received request for getTransceiverInfo, with ids: "
             << (ids->size() > 0 ? folly::join(",", *ids) : "None");
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) |
      folly::gen::appendTo(*ids);
  }

  for (const auto& i : *ids) {
    TransceiverInfo trans;
    if (isValidTransceiver(i)) {
      trans = transceivers_[TransceiverID(i)]->getTransceiverInfo();
    }
    info[i] = trans;
  }
}

void WedgeManager::getTransceiversRawDOMData(
    std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  XLOG(INFO) << "Received request for getTransceiversRawDOMData, with ids: "
             << (ids->size() > 0 ? folly::join(",", *ids) : "None");
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) |
      folly::gen::appendTo(*ids);
  }
  for (const auto& i : *ids) {
    RawDOMData data;
    if (isValidTransceiver(i)) {
      data = transceivers_[TransceiverID(i)]->getRawDOMData();
    }
    info[i] = data;
  }
}

void WedgeManager::customizeTransceiver(int32_t idx, cfg::PortSpeed speed) {
  transceivers_.at(idx)->customizeTransceiver(speed);
}

void WedgeManager::syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) {

  auto groups = folly::gen::from(*ports)
    | folly::gen::filter([](const std::pair<int32_t, PortStatus>& item) {
        return item.second.__isset.transceiverIdx;
      })
    | folly::gen::groupBy([](const std::pair<int32_t, PortStatus>& item) {
        return item.second.transceiverIdx.transceiverId;
      })
    | folly::gen::as<std::vector>();

  for (auto& group : groups) {
    int32_t transceiverIdx = group.key();
    XLOG(INFO) << "Syncing ports of transceiver " << transceiverIdx;

    auto transceiver = transceivers_.at(transceiverIdx).get();
    transceiver->transceiverPortsChanged(group.values());
    info[transceiverIdx] = transceiver->getTransceiverInfo();
  }
}

void WedgeManager::refreshTransceivers() {
  for (const auto& transceiver : transceivers_) {
    transceiver->refresh();
  }
}

std::unique_ptr<BaseWedgeI2CBus> WedgeManager::getI2CBus() {
  return std::make_unique<WedgeI2CBus>();
}

}} // facebook::fboss
