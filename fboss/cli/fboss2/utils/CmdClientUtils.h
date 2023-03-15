/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

namespace facebook::fboss::utils {

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createAgentClient(
    const HostInfo& hostInfo);

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> createQsfpClient(
    const HostInfo& hostInfo);

} // namespace facebook::fboss::utils
