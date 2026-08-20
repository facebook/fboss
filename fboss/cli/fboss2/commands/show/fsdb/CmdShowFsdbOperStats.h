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
class CmdShowFsdbOperStats : public CmdShowFsdbDataCommon {
 public:
  static std::string_view description() {
    return "Displays the FSDB operational STATS subtree at a given path as "
           "JSON, followed by the last confirmed/published/served timestamps. "
           "Requires a path argument (e.g. /agent/hwPortStats). Use it to "
           "inspect published operational counters.";
  }

  static RetType sampleModel() {
    RetType model;
    model.protocol() = fsdb::OperProtocol::SIMPLE_JSON;
    model.contents() =
        R"({"eth1/9/5":{"inBytes_":68170756313545,"inUnicastPkts_":5554033497,"inDiscards_":2072237,"inErrors_":0,"outBytes_":92028875630243,"outUnicastPkts_":24526576207,"outDiscards_":0,"outErrors_":0,"fecCorrectableErrors":1531898813,"fecUncorrectableErrors":24,"portName_":"eth1/9/5","logicalPortId":53}})";

    fsdb::OperMetadata meta;
    meta.lastConfirmedAt() = 1786213548;
    meta.lastPublishedAt() = 1786213549652;
    meta.lastServedAt() = 1786213550445;
    model.metadata() = meta;

    return model;
  }

 private:
  bool isStats() const override {
    return true;
  }
};
} // namespace facebook::fboss
