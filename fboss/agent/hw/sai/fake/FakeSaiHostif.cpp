/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/fake/FakeSaiHostif.h"

#include <folly/logging/xlog.h>
#include <folly/Optional.h>

using facebook::fboss::FakeSai;
using facebook::fboss::FakeHostif;

sai_status_t send_hostif_fn(
    sai_object_id_t /* switch_id */,
    sai_size_t /* buffer_size */,
    const void * /* buffer */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  sai_object_id_t tx_port;
  sai_hostif_tx_type_t tx_type;
  for (int i = 0; i < attr_count; ++i) {
    switch(attr_list[i].id) {
      case SAI_HOSTIF_PACKET_ATTR_HOSTIF_TX_TYPE:
        tx_type = static_cast<sai_hostif_tx_type_t>(attr_list[i].value.s32);
        break;
      case SAI_HOSTIF_PACKET_ATTR_EGRESS_PORT_OR_LAG:
        tx_port = attr_list[i].value.oid;
    }
  }
  XLOG(INFO) << "Sending packet on port : " << std::hex << tx_port <<
                " tx type : " << tx_type;

  return SAI_STATUS_SUCCESS;
}

namespace facebook {
namespace fboss {

static sai_hostif_api_t _hostif_api;

void populate_hostif_api(sai_hostif_api_t** hostif_api) {
  _hostif_api.send_hostif_packet = &send_hostif_fn;
  *hostif_api = &_hostif_api;
}

} // namespace fboss
} // namespace facebook
