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
 public:
  static std::string_view description() {
    return "Displays the FSDB operational STATE subtree at a given path as "
           "JSON, followed by the last confirmed/published/served timestamps. "
           "Requires a path argument (e.g. /agent/switchState/portMaps). Use "
           "it to inspect published operational state.";
  }

  static RetType sampleModel() {
    RetType model;
    model.protocol() = fsdb::OperProtocol::SIMPLE_JSON;
    model.contents() =
        R"({"id=0":{"1":{"portId":1,"portName":"eth1/2/1","portDescription":"neighbor peer001 on eth1/2/1","portState":"ENABLED","portOperState":true,"ingressVlan":2003,"portSpeed":"FOURHUNDREDG","portProfileID":"PROFILE_400G_4_PAM4_RS544X2N_OPTICAL","maxFrameSize":9412}}})";

    fsdb::OperMetadata meta;
    meta.lastConfirmedAt() = 1786213494;
    meta.lastPublishedAt() = 1786213498624;
    meta.lastServedAt() = 1786213499468;
    model.metadata() = meta;

    return model;
  }

 private:
  bool isStats() const override {
    return false;
  }
};
} // namespace facebook::fboss
