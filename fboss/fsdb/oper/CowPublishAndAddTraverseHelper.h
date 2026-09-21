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

#include "fboss/fsdb/oper/SubscriptionPathStore.h"
#include "fboss/fsdb/oper/SubscriptionStore.h"
#include "fboss/thrift_cow/visitors/TraverseHelper.h"

namespace facebook::fboss::fsdb {

struct CowPublishAndAddTraverseHelper
    : thrift_cow::TraverseHelper<CowPublishAndAddTraverseHelper> {
  using Base = thrift_cow::TraverseHelper<CowPublishAndAddTraverseHelper>;

  using Base::path;
  using Base::shouldShortCircuit;

  CowPublishAndAddTraverseHelper(
      SubscriptionPathStore* root,
      SubscriptionStore* store);

  bool shouldShortCircuitImpl(thrift_cow::VisitorType visitorType) const;

  void onPushImpl(thrift_cow::ThriftTCType /* tc */);

  void onPopImpl(std::string&& /* popped */, thrift_cow::ThriftTCType /* tc */);

 private:
  std::vector<SubscriptionPathStore*> pathStores_;
  SubscriptionStore* store_{nullptr};
};

} // namespace facebook::fboss::fsdb
