/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/diag/DiagShell.h"

namespace facebook::fboss {

void DiagShell::logToScuba(
    std::unique_ptr<ClientInformation> /*client*/,
    const std::string& /*input*/,
    const std::optional<std::string>& /*result*/) const {}

} // namespace facebook::fboss
