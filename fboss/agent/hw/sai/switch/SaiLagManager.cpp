// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiLagManager.h"

namespace facebook::fboss {

void SaiLagManager::addLag(
    const std::shared_ptr<AggregatePort>& /*aggregatePort*/) {}

void SaiLagManager::removeLag(
    const std::shared_ptr<AggregatePort>& /*aggregatePort*/) {}

void SaiLagManager::changeLag(
    const std::shared_ptr<AggregatePort>& /*oldAggregatePort*/,
    const std::shared_ptr<AggregatePort>& /*newAggregatePort*/) {}

void SaiLagManager::addMember(
    std::shared_ptr<SaiLag> /*lag*/,
    AggregatePortFields::Subport /*subPort*/) {}

void SaiLagManager::removeMember(
    std::shared_ptr<SaiLag> /*lag*/,
    AggregatePortFields::Subport /*subPort*/) {}
} // namespace facebook::fboss
