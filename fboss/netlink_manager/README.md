# Netlink Manager

Routing protocol daemon like Quagga, BIRD, FRR etc do not talk to FBOSS agent directly, but they do communicate with Linux Kernel via netlink channel. This program eavesdrops on the netlink channel and send thrift calls to FBOSS agent.

Currently, Netlink Manager only supports route updates (NEW & DELETE).

# Build & Run
* Build: netlink_manager is built when you run "make" in [BUILD.md](https://github.com/facebook/fboss/BUILD.md).
* Run:
    Options:
    * -ip: ip address of the switch running FBOSS agent, default to localhost.
    * -fboss_port: FBOSS agent port, default to 5909
    * -interfaces: interfaces to monitored, default to FBOSS interfaces.
    * -debug: enable debug mode (no thrift calls made to FBOSS agent), default to false
    Other useful options:
    * -v: log level. Recommended to use 2.

# Netlink Manager Server & Client
* Cmake file coming
* Allow users to add/remove monitored interfaces to NetlinkManager
* Allow users to add/remove route through thrift client (mainly for testing)
* Code in [NetlinkManagerHandler.cpp](https://github.com/facebook/fboss/blob/master/fboss/netlink_manager/NetlinkManagerHandler.cpp)

# Technical Overview
## EventBase
Netlink Manager uses [folly::EventBase](https://github.com/facebook/folly/blob/master/folly/io/async/README.md) to manage async calls, and [folly::EventHandler](https://github.com/facebook/folly/blob/master/folly/io/async/README.md#eventhandler) to poll netlink socket.

## libnl cache manager & callback
The callback method is NetlinkManager::netlinkRouteUpdated().

This diagram (copied & edited from [infradead](https://www.infradead.org/~tgr/libnl/doc/core.html#core_cb)), explains the role of manager, routeCache(NlResources.cpp) & netlinkRouteUpdated() callback (NetlinkManager.cpp)
```
netlink_manager       libnl                        Kernel

       |                            |

           +-----------------+        [ notification, link change ]

       |   |  nl_cache_mngr  |      | [   (IFF_UP | IFF_RUNNING)  ]

           |                 |                |

       |   |   +------------+|      |         |  [ notification, new addr ]

   <-------|---| routeCache  |<-------(async)--+  [  10.0.1.1/32 dev eth1  ]

       |   |   +------------+|      |                      |

           |   +------------+|                             |

   <---|---|---| linkCache   |<------|-(async)--------------+

           |   +------------+|

       |   |   +------------+|      |

   <-------|---| ...        ||

       |   |   +------------+|      |

           +-----------------+

       |                            |
```
# Todo list:
* Resync state with FBOSS agent after agent restarts
* Support other types of updates aside from routes
