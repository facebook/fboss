// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/tracer/SaiTracer.h"

namespace facebook::fboss {

sai_tam_api_t* wrappedTamApi();

SET_ATTRIBUTE_FUNC_DECLARATION(Tam);
SET_ATTRIBUTE_FUNC_DECLARATION(TamEvent);
SET_ATTRIBUTE_FUNC_DECLARATION(TamEventAction);
SET_ATTRIBUTE_FUNC_DECLARATION(TamReport);
SET_ATTRIBUTE_FUNC_DECLARATION(TamTransport);
SET_ATTRIBUTE_FUNC_DECLARATION(TamCollector);

} // namespace facebook::fboss
