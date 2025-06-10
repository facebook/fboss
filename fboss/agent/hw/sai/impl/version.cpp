// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/impl/version.h"

namespace facebook::fboss {
std::string getSaiImpVersion(bool /* verbose */) {
  return "sai";
}
} // namespace facebook::fboss
