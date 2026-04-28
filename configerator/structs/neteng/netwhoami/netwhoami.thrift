/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Minimal stub for OSS builds
// BGP++ only uses NetWhoAmI.name() in Config.cpp to read the device name from /etc/netwhoami.json.

namespace cpp2 facebook.netwhoami
namespace py neteng.netwhoami
namespace py3 neteng.netwhoami
namespace go neteng.netwhoami

package "facebook.com/neteng/fboss/bgp/public_tld/configerator/structs/neteng/netwhoami/netwhoami"

struct NetWhoAmI {
  1: optional string name;
}
