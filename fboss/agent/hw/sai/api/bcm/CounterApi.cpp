// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/CounterApi.h"

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
std::optional<sai_attr_id_t>
SaiCounterTraits::Attributes::AttributeLabelExtendedWrapper::operator()() {
  return std::nullopt;
}
#endif

} // namespace facebook::fboss
