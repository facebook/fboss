// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/QsfpServiceHandler.h"

#include "fboss/agent/FbossError.h"

#include <fboss/lib/LogThriftCall.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook {
namespace fboss {

QsfpServiceHandler::QsfpServiceHandler(
    std::unique_ptr<TransceiverManager> manager)
    : FacebookBase2("QsfpService"), manager_(std::move(manager)) {}

void QsfpServiceHandler::init() {
  // Initialize the I2c bus
  manager_->initTransceiverMap();
  // Initialize the PhyManager all ExternalPhy for the system
  manager_->initExternalPhyMap();
}

facebook::fb303::cpp2::fb_status QsfpServiceHandler::getStatus() {
  return facebook::fb303::cpp2::fb_status::ALIVE;
}

TransceiverType QsfpServiceHandler::getType(int32_t /* unused */) {
  auto log = LOG_THRIFT_CALL(INFO);
  return TransceiverType::QSFP;
}

void QsfpServiceHandler::getTransceiverInfo(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getTransceiversInfo(info, std::move(ids));
}

void QsfpServiceHandler::customizeTransceiver(
    int32_t idx,
    cfg::PortSpeed speed) {
  auto log = LOG_THRIFT_CALL(INFO);
  XLOG(INFO) << "customizeTransceiver request for " << idx << " to speed "
             << apache::thrift::util::enumNameSafe(speed);
  manager_->customizeTransceiver(idx, speed);
}

void QsfpServiceHandler::getTransceiverRawDOMData(
    std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getTransceiversRawDOMData(info, std::move(ids));
}

void QsfpServiceHandler::getTransceiverDOMDataUnion(
    std::map<int32_t, DOMDataUnion>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getTransceiversDOMDataUnion(info, std::move(ids));
}

void QsfpServiceHandler::syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->syncPorts(info, std::move(ports));
}

void QsfpServiceHandler::pauseRemediation(int32_t timeout) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->setPauseRemediation(timeout);
}

int32_t QsfpServiceHandler::getRemediationUntilTime() {
  auto log = LOG_THRIFT_CALL(INFO);
  return manager_->getPauseRemediationUntil();
}

void QsfpServiceHandler::readTransceiverRegister(
    std::map<int32_t, ReadResponse>& response,
    std::unique_ptr<ReadRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO);
  auto param = *(request->parameter_ref());
  auto offset = *(param.offset_ref());
  if (offset < 0 || offset > 255) {
    throw FbossError("Offset cannot be < 0 or > 255");
  }
  auto page_ref = param.page_ref();
  if (page_ref.has_value() && *page_ref < 0) {
    throw FbossError("Page cannot be < 0");
  }
  auto length_ref = param.length_ref();
  if (length_ref.has_value()) {
    if (*length_ref < 0 || *length_ref > 255) {
      throw FbossError("Length cannot be < 0 or > 255");
    } else if (*length_ref + offset > 256) {
      throw FbossError("Offset + Length cannot be > 256");
    }
  }
  manager_->readTransceiverRegister(response, std::move(request));
}

void QsfpServiceHandler::writeTransceiverRegister(
    std::map<int32_t, WriteResponse>& response,
    std::unique_ptr<WriteRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO);
  auto param = *(request->parameter_ref());
  auto offset = *(param.offset_ref());
  if (offset < 0 || offset > 255) {
    throw FbossError("Offset cannot be < 0 or > 255");
  }
  auto page_ref = param.page_ref();
  if (page_ref.has_value() && *page_ref < 0) {
    throw FbossError("Page cannot be < 0");
  }
  manager_->writeTransceiverRegister(response, std::move(request));
}

void QsfpServiceHandler::programXphyPort(
    int32_t portId,
    cfg::PortProfileID portProfileId) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->programXphyPort(portId, portProfileId);
}

} // namespace fboss
} // namespace facebook
