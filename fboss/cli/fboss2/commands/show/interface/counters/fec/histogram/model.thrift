package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/lib/phy/phy.thrift"

struct nBitCorrectedCount {
  1: map<i16, i64> nBitCorrectedMax;
  2: map<i16, i64> nBitCorrectedAvg;
  3: map<i16, i64> nBitCorrectedCur;
}

struct ShowInterfaceCountersFecHistogramModel {
  1: map<string, nBitCorrectedCount> nBitCorrectedWords;
  10: phy.Direction direction;
}
