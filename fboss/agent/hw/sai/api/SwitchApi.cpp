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

sai_status_t SwitchApi::registerRxCallback(
    SwitchSaiId id,
    sai_packet_event_notification_fn rx_cb) {
  sai_attribute_t attr;
  attr.id = SAI_SWITCH_ATTR_PACKET_EVENT_NOTIFY;
  attr.value.ptr = (void*)rx_cb;
  return _setAttribute(id, &attr);
}

sai_status_t SwitchApi::registerPortStateChangeCallback(
    SwitchSaiId id,
    sai_port_state_change_notification_fn port_state_change_cb) {
  sai_attribute_t attr;
  attr.id = SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY;
  attr.value.ptr = (void*)port_state_change_cb;
  return _setAttribute(id, &attr);
}

sai_status_t SwitchApi::registerFdbEventCallback(
    SwitchSaiId id,
    sai_fdb_event_notification_fn fdb_event_cb) {
  sai_attribute_t attr;
  attr.id = SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY;
  attr.value.ptr = (void*)fdb_event_cb;
  return _setAttribute(id, &attr);
}

} // namespace facebook::fboss
