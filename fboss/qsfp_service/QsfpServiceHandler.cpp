#include "fboss/qsfp_service/QsfpServiceHandler.h"

#include <fboss/lib/LogThriftCall.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook { namespace fboss {

QsfpServiceHandler::QsfpServiceHandler(
  std::unique_ptr<TransceiverManager> manager) :
    FacebookBase2("QsfpService"),
    manager_(std::move(manager)) {
}

void QsfpServiceHandler::init() {
  manager_->initTransceiverMap();
}

facebook::fb303::cpp2::fb_status QsfpServiceHandler::getStatus() {
  return facebook::fb303::cpp2::fb_status::ALIVE;
}

TransceiverType QsfpServiceHandler::getType(int32_t /* unused */) {
  auto log = LOG_THRIFT_CALL(INFO);
  return TransceiverType::QSFP;
}

void QsfpServiceHandler::getTransceiverInfo(std::map<int32_t,
    TransceiverInfo>& info, std::unique_ptr<std::vector<int32_t>> ids) {
  auto log = LOG_THRIFT_CALL(INFO);
  manager_->getTransceiversInfo(info, std::move(ids));
}

void QsfpServiceHandler::customizeTransceiver(int32_t idx,
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

}} // facebook::fboss
