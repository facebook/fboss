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

#include "common/stats/ExportedTimeseries.h"
#include "common/stats/ExportedHistogram.h"
#include <map>

namespace facebook { namespace stats {

class ServiceData {
public:
  ExportedStatMap* getStatMap() {
    static ExportedStatMap it;
    return &it;
  }
  ExportedHistogramMap* getHistogramMap() {
    static ExportedHistogramMap it;
    return &it;
  }
  void getCounters(std::map<std::string, int64_t>&) {}
  void setUseOptionsAsFlags(bool) {}
  void setCounter(folly::StringPiece, uint32_t) {}
};

}

extern stats::ServiceData* fbData;

}
