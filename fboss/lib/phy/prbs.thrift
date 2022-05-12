#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.prbs
namespace py3 neteng.fboss.prbs
namespace py.asyncio neteng.fboss.asyncio.prbs
namespace cpp2 facebook.fboss.prbs
namespace go neteng.fboss.prbs
namespace php fboss_prbs

enum PrbsPolynomial {
  PRBS7 = 7,
  PRBS9 = 9,
  PRBS10 = 10,
  PRBS11 = 11,
  PRBS13 = 13,
  PRBS15 = 15,
  PRBS20 = 20,
  PRBS23 = 23,
  PRBS31 = 31,
  PRBS49 = 49,
  PRBS58 = 58,
  // Gray encoded patterns for PAM4 (assigning 1xx value for differentiation)
  PRBS7Q = 107,
  PRBS9Q = 109,
  PRBS13Q = 113,
  PRBS15Q = 115,
  PRBS23Q = 123,
  PRBS31Q = 131,
  // Misc
  PRBSSSPRQ = 200,
}

struct InterfacePrbsState {
  1: PrbsPolynomial polynomial;
  2: optional bool generatorEnabled;
  3: optional bool checkerEnabled;
}
