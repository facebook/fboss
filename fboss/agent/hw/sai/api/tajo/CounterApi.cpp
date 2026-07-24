// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/CounterApi.h"

extern "C" {
#if defined(TAJO_SDK_GTE_26_5)
#include <saiextensions.h>
#else
#include <experimental/sai_attr_ext.h>
#endif
}

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
std::optional<sai_attr_id_t>
SaiCounterTraits::Attributes::AttributeLabelExtendedWrapper::operator()() {
#if defined(TAJO_SDK_VERSION_26_2_4210) || defined(TAJO_SDK_VERSION_26_5_5210)
  return SAI_COUNTER_ATTR_EXT_LABEL_EXTENDED;
#else
  return std::nullopt;
#endif
}
#endif

} // namespace facebook::fboss
