#pragma once

/* C headers */
extern "C" {
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_tun.h>
#include <libnl3/netlink/netlink.h>
#include <libnl3/netlink/types.h>
#include <libnl3/netlink/cache.h>
#include <libnl3/netlink/route/link.h>
#include <libnl3/netlink/route/route.h>
#include <libnl3/netlink/route/neighbour.h>
#include <libnl3/netlink/route/addr.h>
#include <sys/epoll.h>
}
/* C++ headers */
#include <string>
#include <iostream>
#include <memory> /* std::unique_ptr */
#include <boost/thread.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include "TapIntf.h"

#include <folly/io/async/EventBase.h>

#include "fboss/agent/types.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/TxPacket.h"

/* Buffer used to read in packets from host */
#define BUFLEN 65536

namespace facebook { namespace fboss {

class SwSwitch;

class NetlinkListener
{
	public:
	NetlinkListener(SwSwitch * sw, folly::EventBase * evb, std::string &iface_prefix);
	~NetlinkListener();

	/* Call first to add Interfaces to the SwitchState */
	void addInterfacesAndUpdateState(std::shared_ptr<SwitchState> swState);
	/* Call second to add TapIntf tap interfaces to host */
	void startNetlinkListener(const int pollIntervalMillis);
	/* Kill listener thread and remove tap interfaces*/
	void stopNetlinkListener();

	inline const std::map<int, TapIntf *>& getInterfacesByIfindex()
	{
		return interfaces_by_ifindex_;
	};

	inline const std::map<VlanID, TapIntf *>& getInterfacesByVlanID()
	{
		return interfaces_by_vlan_;
	};

	bool sendPacketToHost(std::unique_ptr<RxPacket> pkt);

	private:

	/* Our variables */
	struct nl_dump_params dump_params_;		/* format and verbosity of netlink cache dumps */
	struct nl_sock * sock_; 			/* pipe to RX/TX netlink messages */
	struct nl_cache * link_cache_;			/* our copy of the system link state */
	struct nl_cache * route_cache_;			/* our copy of the system route state */
	struct nl_cache * neigh_cache_;			/* our copy of the system ARP table */
	struct nl_cache * addr_cache_;			/* our copy of the system L3 addresses */
	struct nl_cache_mngr * manager_;		/* wraps caches and notifies us upon a change */
	std::string prefix_;				/* name we'll use for our host tap interfaces */
	std::map<int, TapIntf *> interfaces_by_ifindex_;
	std::map<VlanID, TapIntf *> interfaces_by_vlan_;
	boost::thread * netlink_listener_thread_;	/* polls cache manager for updates */
	boost::thread * host_packet_rx_thread_;		/* polls host iface FDs for packets en route to the dataplane */
	SwSwitch * sw_;					/* use to update state and (TODO) receive updates */
	folly::EventBase * evb_;			/* TODO consider removing this. I don't think we need it */

	/* No copy or assign */
	NetlinkListener(const NetlinkListener &);
	NetlinkListener& operator=(const NetlinkListener &);

	/* Logging -- TODO not necessary with FBOSS's logging */
	void log_and_die(const char * msg);
	void log_and_die_rc(const char * msg, const int rc);

	/* Netlink logging verbosity in object dumps */
	void init_dump_params();

	/* Used internally when dumping Netlink nl_object types */
	struct nl_dump_params * get_dump_params();
	
	/* Monitor what the host is doing (network-wise) using netlink */
	void register_w_netlink();
	void unregister_w_netlink();
	
	/* 
	 * interfaces:
	 *
	 * The term "interface" is thrown around a lot but means different
	 * things and refers to different types in different places. Internally,
	 * we use TapIntf to represent the FBOSS-to-host-OS information that
	 * enables a tap interface. FBOSS uses Interface to track a host OS
	 * interface the FBOSS agent is supposed to be emulating. In non-netlink
	 * mode, this emulation will occur (e.g. ARP handling, etc.), but in 
	 * netlink mode, we delegate this to the host OS. Still, FBOSS tracks
	 * the SwitchState using Interfaces. Thus, we need to map between the
	 * FBOSS Interface and our TapIntf.
	 *
	 * These functions are responsible for handling both TapIntf and
	 * Interface interfaces.
	 */
	void add_ifaces(const std::string &prefix, std::shared_ptr<SwitchState> state);
	void delete_ifaces();

	/* 
	 * Netlink callbacks:
	 *
	 * nl_operation can be one of NL_ACT_NEW, NL_ACT_CHANGE, or NL_ACT_DEL for add, modify, and remove operations
	 * data is used to pass in 'this' NetlinkListener
	 */
	static void netlink_link_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data);
	static void netlink_route_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data);
	static void netlink_neighbor_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data);
	static void netlink_address_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data);

	/* Cache manager polling thread */
	static void netlink_listener(const int pollIntervalMillis, NetlinkListener * nll);

	/* Get ready to receive packets from the host on a tap interface */
	void init_host_packet_rx();

	/* Handle packets received from host on a tap interface */
	static void host_packet_rx_listener(NetlinkListener * nll);
	int read_packet_from_port(NetlinkListener * nll, TapIntf * iface);

};

}} /* facebook::fboss */
