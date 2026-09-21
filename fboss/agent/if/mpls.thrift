include "thrift/annotation/thrift.thrift"
include "thrift/annotation/hack.thrift"

@hack.NamePrefix{prefix = "fboss_mpls_"}
@hack.LegacyOmitPrefixInNameString
@hack.ConstantsClass{name = "fboss_mpls_CONSTANTS"}
@thrift.AllowLegacyMissingUris
package;

namespace cpp2 facebook.fboss
namespace go neteng.fboss.mpls
namespace py neteng.fboss.mpls
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.mpls

typedef i32 MplsLabel

// label is 20 bit in MPLS header
const MplsLabel MAX_MPLS_LABEL = 0xfffff;

// first element represents bottom of stack, last element represents top of stack
typedef list<i32> MplsLabelStack

enum MplsActionCode {
  # Forward via an MPLS next hop that imposes MplsAction.pushLabels.
  # On an IP route, this turns the IP packet into an MPLS packet. On a label
  # route, FBOSS first pops the matched top label, then imposes pushLabels.
  PUSH = 0,

  # Replace the matched top label with MplsAction.swapLabel and forward via the
  # resolved next hop.
  SWAP = 1,

  # Pop one matched top label and forward via the resolved next hop with no
  # label imposition. If the ingress packet has more labels, the remaining
  # label stack is preserved.
  #
  # Note: Historically named PHP. In FBOSS this action is generic
  # pop-and-forward; it is not limited to classic penultimate-hop popping to IP.
  PHP = 2,

  # Pop one matched top label and continue forwarding by looking up the exposed
  # payload/header, which is expected to be an IP header in current FBOSS agent
  # usage, instead of forwarding directly via the MPLS route nexthop.
  POP_AND_LOOKUP = 3,

  NOOP = 4,
}

struct MplsAction {
  1: MplsActionCode action;
  2: optional MplsLabel swapLabel; // Required if action == SWAP
  3: optional MplsLabelStack pushLabels; // Required if action == PUSH
}
