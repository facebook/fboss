/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/SwitchApi.h"

namespace facebook::fboss {

void SwitchApi::registerRxCallback(
    SwitchSaiId id,
    sai_packet_event_notification_fn rx_cb) const {
  sai_attribute_t attr;
  attr.id = SAI_SWITCH_ATTR_PACKET_EVENT_NOTIFY;
  attr.value.ptr = (void*)rx_cb;
  auto rv = _setAttribute(id, &attr);
  saiApiCheckError(
      rv,
      ApiType,
      "Unable to ",
      rx_cb ? "register" : "unregister",
      " rx callback");
}

void SwitchApi::registerPortStateChangeCallback(
    SwitchSaiId id,
    sai_port_state_change_notification_fn port_state_change_cb) const {
  sai_attribute_t attr;
  attr.id = SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY;
  attr.value.ptr = (void*)port_state_change_cb;
  auto rv = _setAttribute(id, &attr);
  saiApiCheckError(
      rv,
      ApiType,
      "Unable to ",
      port_state_change_cb ? "register" : "unregister",
      " port state change callback");
}

void SwitchApi::registerFdbEventCallback(
    SwitchSaiId id,
    sai_fdb_event_notification_fn fdb_event_cb) const {
  sai_attribute_t attr;
  attr.id = SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY;
  attr.value.ptr = (void*)fdb_event_cb;
  auto rv = _setAttribute(id, &attr);
  saiApiCheckError(
      rv,
      ApiType,
      "Unable to ",
      fdb_event_cb ? "register" : "unregister",
      " FDB event callback");
}

void SwitchApi::registerTamEventCallback(
    SwitchSaiId id,
    sai_tam_event_notification_fn tam_event_cb) const {
  sai_attribute_t attr;
  attr.id = SAI_SWITCH_ATTR_TAM_EVENT_NOTIFY;
  attr.value.ptr = (void*)tam_event_cb;
  auto rv = _setAttribute(id, &attr);
  saiLogError(
      rv,
      ApiType,
      "Unable to ",
      tam_event_cb ? "register" : "unregister",
      " TAM event callback");
}

void SwitchApi::registerQueuePfcDeadlockNotificationCallback(
    SwitchSaiId id,
    sai_queue_pfc_deadlock_notification_fn queue_pfc_deadlock_notification_cb)
    const {
  sai_attribute_t attr;
  attr.id = SAI_SWITCH_ATTR_QUEUE_PFC_DEADLOCK_NOTIFY;
  attr.value.ptr = (void*)queue_pfc_deadlock_notification_cb;
  auto rv = _setAttribute(id, &attr);
  saiLogError(
      rv,
      ApiType,
      "Unable to ",
      queue_pfc_deadlock_notification_cb ? "register" : "unregister",
      " Queue PFC deadlock notification callback");
}

} // namespace facebook::fboss
