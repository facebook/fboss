/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/ForwardingInformationBaseDelta.h"

namespace facebook::fboss {

thrift_cow::ThriftMapDelta<ForwardingInformationBaseV4>
ForwardingInformationBaseContainerDelta::getV4FibDelta() const {
  return thrift_cow::ThriftMapDelta<ForwardingInformationBaseV4>(
      getOld() ? getOld()->getFibV4().get() : nullptr,
      getNew() ? getNew()->getFibV4().get() : nullptr);
}

thrift_cow::ThriftMapDelta<ForwardingInformationBaseV6>
ForwardingInformationBaseContainerDelta::getV6FibDelta() const {
  return thrift_cow::ThriftMapDelta<ForwardingInformationBaseV6>(
      getOld() ? getOld()->getFibV6().get() : nullptr,
      getNew() ? getNew()->getFibV6().get() : nullptr);
}

template class thrift_cow::ThriftMapDelta<ForwardingInformationBaseV4>;
template class thrift_cow::ThriftMapDelta<ForwardingInformationBaseV6>;
template class NodeMapDelta<
    ForwardingInformationBaseMap,
    ForwardingInformationBaseContainerDelta>;

} // namespace facebook::fboss
