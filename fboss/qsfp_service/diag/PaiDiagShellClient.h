/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once

#include <folly/IPAddress.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <cstdint>
#include <memory>

#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

DECLARE_bool(help);

namespace facebook::fboss {

/*
 * Build a thrift async client for qsfp_service that supports server-streaming
 * (used by PAI diag shell's startPaiDiagShell endpoint).
 *
 * Two implementations live in oss/ and facebook/:
 *   - oss/PaiDiagShellClient.cpp    : raw AsyncSocket + RocketClientChannel
 *   - facebook/PaiDiagShellClient.cpp : ServiceRouter w/ cert-aware fallback
 *
 * Mirrors fboss/agent/hw/sai/diag/DiagShellClient's getStreamingClient(),
 * which is the production-vetted pattern for the BCM NPU diag shell.
 */
std::unique_ptr<apache::thrift::Client<QsfpService>> getPaiDiagStreamingClient(
    folly::EventBase* evb,
    const folly::IPAddress& ip,
    uint32_t port);

} // namespace facebook::fboss
