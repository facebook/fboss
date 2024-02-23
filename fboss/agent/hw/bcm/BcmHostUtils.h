// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

namespace facebook::fboss {

class BcmSwitch;
class BcmHostKey;

void setDisableTTLDecrement(const BcmSwitch* hw, const BcmHostKey& key);

} // namespace facebook::fboss
