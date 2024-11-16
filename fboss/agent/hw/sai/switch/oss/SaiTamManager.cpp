// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiTamManager.h"

namespace facebook::fboss {
SaiTamHandle::~SaiTamHandle() {}

SaiTamManager::SaiTamManager(
    SaiStore* /* saiStore */,
    SaiManagerTable* /*managerTable*/,
    SaiPlatform* /*platform*/) {}

void SaiTamManager::addMirrorOnDropReport(
    const std::shared_ptr<MirrorOnDropReport>& /* report */) {}

void SaiTamManager::removeMirrorOnDropReport(
    const std::shared_ptr<MirrorOnDropReport>& /* report */) {}

void SaiTamManager::changeMirrorOnDropReport(
    const std::shared_ptr<MirrorOnDropReport>& /* oldReport */,
    const std::shared_ptr<MirrorOnDropReport>& /* newReport */) {}

} // namespace facebook::fboss
