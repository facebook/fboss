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

#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbDataCommon.h"

namespace facebook::fboss {
class CmdShowFsdbOperState : public CmdShowFsdbDataCommon {
 private:
  bool isStats() const override {
    return false;
  }
};
} // namespace facebook::fboss
