/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

using facebook::fboss::PortQueue;
using facebook::fboss::QueueApiParameters;
using facebook::fboss::SaiApiTable;
using facebook::fboss::cfg::StreamType;

namespace {
QueueApiParameters::Attributes getSaiQueueAttributes(
    SaiApiTable* apiTable,
    sai_object_id_t saiQueueId) {
  auto& queueApi = apiTable->queueApi();
  auto type =
      queueApi.getAttribute(QueueApiParameters::Attributes::Type(), saiQueueId);
  auto queueId = queueApi.getAttribute(
      QueueApiParameters::Attributes::Index(), saiQueueId);
  auto portId =
      queueApi.getAttribute(QueueApiParameters::Attributes::Port(), saiQueueId);
  auto parentSchedulerNode = queueApi.getAttribute(
      QueueApiParameters::Attributes::ParentSchedulerNode(), saiQueueId);
  return QueueApiParameters::Attributes(
      {type, portId, queueId, parentSchedulerNode});
}

QueueApiParameters::Attributes getQueueAttributes(
    sai_object_id_t portId,
    const PortQueue& portQueue) {
  auto type = portQueue.getStreamType() == StreamType::UNICAST
      ? SAI_QUEUE_TYPE_UNICAST
      : SAI_QUEUE_TYPE_MULTICAST;
  return QueueApiParameters::Attributes(
      {type, portId, portQueue.getID(), portId});
}
} // namespace

namespace facebook {
namespace fboss {

SaiQueue::SaiQueue(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    sai_object_id_t saiPortId,
    const QueueApiParameters::Attributes& attributes)
    : apiTable_(apiTable), saiPortId_(saiPortId), attributes_(attributes) {
  auto& queueApi = apiTable_->queueApi();
  auto switchId = managerTable->switchManager().getSwitchSaiId();
  id_ = queueApi.create(attributes.attrs(), switchId);
}

SaiQueue::SaiQueue(
    SaiApiTable* apiTable,
    sai_object_id_t id,
    sai_object_id_t saiPortId,
    const QueueApiParameters::Attributes& attributes)
    : apiTable_(apiTable),
      id_(id),
      saiPortId_(saiPortId),
      attributes_(attributes) {}

SaiQueue::~SaiQueue() {
  auto& queueApi = apiTable_->queueApi();
  queueApi.remove(id_);
}

bool SaiQueue::operator==(const SaiQueue& other) const {
  return attributes_ == other.attributes_;
}

bool SaiQueue::operator!=(const SaiQueue& other) const {
  return !(*this == other);
}

void SaiQueue::updatePortQueue(PortQueue& portQueue) {
  auto queueAttr = getQueueAttributes(getSaiPortId(), portQueue);
  if (queueAttr == attributes()) {
    return;
  }
  // TODO(srikrishnagopu): Update Buffer and scheduler attributes
}

SaiQueueManager::SaiQueueManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : apiTable_(apiTable), managerTable_(managerTable), platform_(platform) {}

std::unique_ptr<SaiQueue> SaiQueueManager::createQueue(
    sai_object_id_t saiPortId,
    uint32_t numQueues,
    PortQueue& portQueue) {
  auto queueAttr = getQueueAttributes(saiPortId, portQueue);
  std::vector<sai_object_id_t> ql;
  ql.resize(numQueues);
  PortApiParameters::Attributes::QosQueueList queueListAttribute(ql);
  auto& portApi = apiTable_->portApi();
  auto queueIdList = portApi.getAttribute(queueListAttribute, saiPortId);
  /*
   * queueIdList is the list of all queues created by the SDK for a port.
   * getSaiQueueAttributes populates the SAI attributes like queue ID, queue
   * type(UC/MC) given the sai queue id. This helps us compare against
   * the queue ID we are trying to configure through the config.
   * There can be three possibilities
   * 1) queueIdList is empty: Port came up but the queues are not created
   * in the SDK and the config drives the queue creation.
   * 2) queueIdList is not empty: SDK implicitly created the SAI queue objects.
   * Now, query them, compare it against the agent config and associate the
   * sai queue object with SaiQueue in hardware switch. This is applicable
   * only when the queue attribute matches the configuration. Note that, this
   * will not trigger a SDK call.
   * 3) TODO(srikrishnagopu) queueIdList is not empty and does not match the
   * agent config: This is a special case of 2) and a dangerous one which is
   * not handled yet. SDK creates all the SAI queues but it does not match the
   * agent config. For eg, underlying SDK created all queues as MC queues but
   * agent config is trying to create UC queues.
   */
  for (auto queueId : queueIdList) {
    auto saiQueueAttr = getSaiQueueAttributes(apiTable_, queueId);
    if (queueAttr == saiQueueAttr) {
      return std::make_unique<SaiQueue>(
          apiTable_, queueId, saiPortId, queueAttr);
    }
  }
  return std::make_unique<SaiQueue>(
      apiTable_, managerTable_, saiPortId, queueAttr);
}

} // namespace fboss
} // namespace facebook
