==============================
Facebook Open Switching System
==============================

FBOSS is Facebook's software stack for controlling and managing OCP network
switches.

The central component of FBOSS is the agent daemon, which runs on each switch,
and controls the hardware forwarding ASIC.  The agent daemon sends forwarding
information to the hardware, and implements some control plane protocols such
as ARP and NDP.  The agent provides thrift APIs for managing routes, to allow
external routing control processes to get their routing information programmed
into the hardware forwarding tables.

FBOSS has been designed specifically to handle the needs of Facebook's data
center networks.  We hope it can be used as a starting point for other users
wishing to use OCP switches in their environments.  However, note that it will
likely require additional development work to easily fit into other network
environments.  Until it matures more, FBOSS will likely be primarily of
interest to network software developers, rather than to network administrators
who are hoping to use it as an turnkey solution.

## Dependencies

FBOSS depends on the following libraries, and their dependencies:

* facebook/fbthrift
* facebook/folly

## Missing Pieces

FBOSS is still under heavy development, and we are also working to open source
more of our tools and utilities for managing FBOSS.  The initial open source
release consists primarily of the agent daemon.  There are a few components
that are not (yet) part of the open source release:

### Routing Daemon

The FBOSS release does not yet contain a routing protocol daemon to talk to the
FBOSS agent.  The routing protocol daemon we use at Facebook is rather specific
to our environment, and likely won't be as useful to the open source community.
For the open source community it should be possible to modify an existing open
source daemon like quagga to call the FBOSS thrift APIs, but we have not
implemented this yet.  In the meantime, we have included a small sample python
script in fboss/agent/tools that can manually add and remove routes.

### Management Tools

The current code release has relatively few tools for interacting with and
debugging the fboss agent.  We have some tools that we are working to include
in the open source repository soon.  These didn't make it into the initial
release simply due to time constraints.  In the meantime, the thrift compiler
can automatically generate a python-remote script to allow manual invocation of
the agent's various thrift interfaces.
