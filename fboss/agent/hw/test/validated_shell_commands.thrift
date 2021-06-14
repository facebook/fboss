namespace cpp2 facebook.fboss
namespace py3 neteng.fboss

namespace cpp2 fboss.agent.hw.test.dataplane_tests

const set<string> _BCM_COMMON_TESTED_CMDS = [
  "ps",
  "show count",
  "soc 0",
  "show interrupts",
  "xphy ver phy_id=0",
  "xphy lw_dsc phy_id=0x40 if_side=1 lane_mask=0x11",
]
// Wedge40
const set<string> TD2_TESTED_CMDS = _BCM_COMMON_TESTED_CMDS
// Wedge100, Galaxy
const set<string> TH_TESTED_CMDS = _BCM_COMMON_TESTED_CMDS
// MP, YAMP, W400
const set<string> TH3_TESTED_CMDS = _BCM_COMMON_TESTED_CMDS
// MP2, YAMP2 - TODO
const set<string> TH4_TESTED_CMDS = []
const set<string> TAJO_TESTED_CMDS = []
