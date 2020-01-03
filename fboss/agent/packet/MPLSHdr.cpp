// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/packet/MPLSHdr.h"

namespace facebook::fboss {

MPLSHdr::MPLSHdr(folly::io::Cursor* cursor) {
  MPLSLabel mplsLabel;
  while (cursor->tryReadBE<uint32_t>(mplsLabel.u32)) {
    stack_.push_back(mplsLabel.label);
    if (mplsLabel.label.bottomOfStack) {
      break;
    }
  }
}

void MPLSHdr::serialize(folly::io::RWPrivateCursor* cursor) const {
  MPLSLabel mplsLabel;
  for (auto label : stack_) {
    mplsLabel.label = label;
    cursor->writeBE<uint32_t>(mplsLabel.u32);
  }
}

} // namespace facebook::fboss
