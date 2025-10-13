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

#include "fboss/cli/fboss2/CmdHandler.h"

#include <thrift/lib/cpp/transport/TTransportException.h>
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/show/cpuport/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

#include <unistd.h>
#include <algorithm>

namespace facebook::fboss {

using utils::Table;

struct CmdShowCpuPortTraits : public ReadCommandTraits {
  using RetType = cli::ShowCpuPortModel;
};

class CmdShowCpuPort : public CmdHandler<CmdShowCpuPort, CmdShowCpuPortTraits> {
 public:
  using RetType = CmdShowCpuPortTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& model, std::ostream& out = std::cout);

  RetType createModel(
      std::map<int32_t, facebook::fboss::CpuPortStats>& cpuPortStats);
};

} // namespace facebook::fboss
