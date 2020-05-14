/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmConfig.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/gen-cpp2/bcm_config_types.h"

#include <folly/FileUtil.h>
#include <folly/gen/String.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

DEFINE_string(
    bcm_config,
    "",
    "The location of the Broadcom JSON configuration file");

using folly::StringPiece;
using std::string;

namespace facebook::fboss {

BcmConfig::ConfigMap BcmConfig::loadDefaultConfig() {
  CHECK(!FLAGS_bcm_config.empty()) << "Must set bcm config";
  return loadFromFile(FLAGS_bcm_config);
}

BcmConfig::ConfigMap BcmConfig::loadFromFile(const std::string& path) {
  string contents;
  if (!folly::readFile(path.c_str(), contents)) {
    throw FbossError("unable to read Broadcom config file ", path);
  }

  bcm::BcmConfig cfg;

  // Try parsing it as JSON first.
  try {
    cfg = apache::thrift::SimpleJSONSerializer::deserialize<bcm::BcmConfig>(
        contents);
  } catch (const std::exception& jsonEx) {
    // Wasn't valid json.  Try parsing it as a Broadcom-style flat file
    // instead.  This is mainly just for convenience, so that testing in the
    // lab can use a Broadcom-style file.
    try {
      return parseBcmStyleConfig(contents);
    } catch (const std::exception& flatEx) {
      XLOG(ERR) << "unable to parse " << FLAGS_bcm_config
                << " as either JSON or a BCM-style config";
      XLOG(ERR) << "JSON error: " << folly::exceptionStr(jsonEx);
      XLOG(ERR) << "BCM-style error: " << folly::exceptionStr(flatEx);
      throw jsonEx;
    }
  }

  return *cfg.config_ref();
}

namespace {
void trimStr(StringPiece* value) {
  while (!value->empty() && isspace(value->front())) {
    value->pop_front();
  }
  while (!value->empty() && isspace(value->back())) {
    value->pop_back();
  }
}
} // namespace

BcmConfig::ConfigMap BcmConfig::parseBcmStyleConfig(StringPiece data) {
  ConfigMap results;

  int lineNum = 0;
  auto processLine = [&](StringPiece line) {
    ++lineNum;
    trimStr(&line);
    if (line.empty() || line.front() == '#') {
      return;
    }

    size_t idx = line.find('=');
    if (idx == string::npos) {
      throw FbossError("expected <name>=<value> on line ", lineNum);
    }
    auto name = line.subpiece(0, idx);
    auto value = line.subpiece(idx + 1);
    trimStr(&name);
    trimStr(&value);

    XLOG(DBG3) << "BCM config: " << name << "=" << value;
    results[name.str()] = value.str();
  };

  folly::gen::split(data, "\n") | processLine;
  return results;
}

} // namespace facebook::fboss
