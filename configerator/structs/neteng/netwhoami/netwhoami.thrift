/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
