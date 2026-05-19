// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/cpo/CpoDomMonitor.h"

#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"

namespace facebook {
namespace fboss {

CpoDomMonitor::CpoDomMonitor(TransceiverManager* tcvrMgr)
    : tcvrMgr_(tcvrMgr) {}

void CpoDomMonitor::refreshCpoDomData() {
  if (!tcvrMgr_) {
    return;
  }

  auto transceivers = tcvrMgr_->getTransceivers();
  for (auto& [id, tcvr] : transceivers) {
    auto* cmisModule = dynamic_cast<CmisModule*>(tcvr.get());
    if (!cmisModule) {
      continue;
    }

    auto type = cmisModule->type();
    if (type != TransceiverType::CPO_OE && type != TransceiverType::ELSFP) {
      continue;
    }

    try {
      auto cpoDom = cmisModule->getCpoDomData();
      std::lock_guard<std::mutex> lock(cpoDomMutex_);
      cpoDomCache_[id] = cpoDom;
    } catch (const std::exception& ex) {
      // Skip this module if DOM data cannot be read
    }
  }
}

CpoDomData CpoDomMonitor::getCpoDomData(TransceiverID tcvrId) const {
  std::lock_guard<std::mutex> lock(cpoDomMutex_);
  auto it = cpoDomCache_.find(tcvrId);
  if (it != cpoDomCache_.end()) {
    return it->second;
  }
  return CpoDomData();
}

std::map<TransceiverID, CpoDomData> CpoDomMonitor::getAllCpoDomData() const {
  std::lock_guard<std::mutex> lock(cpoDomMutex_);
  return cpoDomCache_;
}

bool CpoDomMonitor::isCpoModule(TransceiverID tcvrId) const {
  if (!tcvrMgr_) {
    return false;
  }
  auto tcvr = tcvrMgr_->getTransceiver(tcvrId);
  if (!tcvr) {
    return false;
  }
  auto type = tcvr->type();
  return type == TransceiverType::CPO_OE || type == TransceiverType::ELSFP;
}

void CpoDomMonitor::setDomPollingInterval(std::chrono::milliseconds interval) {
  domPollingInterval_ = interval;
}

} // namespace fboss
} // namespace facebook
