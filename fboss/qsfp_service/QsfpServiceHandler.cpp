#include "fboss/qsfp_service/QsfpServiceHandler.h"

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

TransceiverType QsfpServiceHandler::type(int32_t /* unused */) {
  return TransceiverType::QSFP;
}

void QsfpServiceHandler::getTransceiverInfo(std::map<int32_t,
    TransceiverInfo>& info, std::unique_ptr<std::vector<int32_t>> ids) {
  manager_->getTransceiversInfo(info, std::move(ids));
}

void QsfpServiceHandler::customizeTransceiver(int32_t idx,
    cfg::PortSpeed speed) {
  LOG(INFO) << "customizeTransceiver request for " << idx <<
    " to speed " << cfg::_PortSpeed_VALUES_TO_NAMES.find(speed)->second;
  manager_->customizeTransceiver(idx, speed);
}

void QsfpServiceHandler::getTransceiverRawDOMData(
    std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  manager_->getTransceiversRawDOMData(info, std::move(ids));
}

void QsfpServiceHandler::syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) {
  manager_->syncPorts(info, std::move(ports));
}

}} // facebook::fboss
