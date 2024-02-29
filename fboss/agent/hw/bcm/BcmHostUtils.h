// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

namespace facebook::fboss {

class BcmSwitch;
class BcmHostKey;

/* if TTL should be decremeneted or not, noDecrement is true, TTL is not
 * decremented, otherwise it is decremented */
void setTTLDecrement(
    const BcmSwitch* hw,
    const BcmHostKey& key,
    bool noDecrement);

} // namespace facebook::fboss
