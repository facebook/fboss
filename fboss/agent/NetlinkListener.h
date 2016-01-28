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

/* Buffer used to read in packets from host */
#define BUFLEN 65536

namespace facebook { namespace fboss {

class SwSwitch;

class NetlinkListener
{
	public:
	NetlinkListener(SwSwitch * sw, folly::EventBase * evb, std::string &iface_prefix);
	~NetlinkListener();

	void startNetlinkListener(const int pollIntervalMillis, std::shared_ptr<SwitchState> swState);
	void stopNetlinkListener();

	private:

	/* variables */
	struct nl_dump_params dump_params_;		/* format and verbosity of netlink cache dumps */
	struct nl_sock * sock_; 			/* pipe to RX/TX netlink messages */
	struct nl_cache * link_cache_;			/* our copy of the system link state */
	struct nl_cache * route_cache_;			/* our copy of the system route state */
	struct nl_cache_mngr * manager_;		/* wraps caches and notifies us upon a change */
	std::string prefix_;
	std::map<int, TapIntf *> interfaces_by_ifindex_;
	boost::thread * netlink_listener_thread_;	/* polls cache manager for updates */
	boost::thread * host_packet_rx_thread_;		/* polls host iface FDs for packets en route to the dataplane */
	SwSwitch * sw_;
	folly::EventBase * evb_;

	/* no copy or assign */
	NetlinkListener(const NetlinkListener &);
	NetlinkListener& operator=(const NetlinkListener &);

	inline const std::map<int, TapIntf *>& get_interfaces()
	{
		return interfaces_by_ifindex_;
	};

	/* logging */
	void log_and_die(const char * msg);
	void log_and_die_rc(const char * msg, const int rc);

	/* initialize data structures */
	void register_w_netlink();
	void unregister_w_netlink();
	void init_dump_params();
	struct nl_dump_params * get_dump_params();

	/* interfaces */
	void add_ifaces(const std::string &prefix, const int qty);
	void delete_ifaces();
	void init_host_packet_rx();
	int read_packet_from_port(NetlinkListener * nll, TapIntf * iface);


	/* netlink callbacks */
	static void netlink_link_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data);
	static void netlink_route_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data);

	/* cache manager polling thread */
	static void netlink_listener(const int pollIntervalMillis, NetlinkListener * nll);
	static void host_packet_rx_listener(NetlinkListener * nll);
};

}} /* facebook::fboss */
