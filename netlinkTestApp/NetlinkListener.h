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
}
/* C++ headers */
#include <string>
#include <iostream>
#include <memory> /* std::unique_ptr */
#include <boost/thread.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include "TapIntf.h"

class NetlinkListener
{
	public:
	NetlinkListener(const std::string &iface_prefix, const int iface_qty); /* add SwSwitch */
	~NetlinkListener();

	void startListening(int pollIntervalMillis);
	void stopListening();

	private:

	/* variables */
	struct nl_dump_params dump_params_;		/* format and verbosity of netlink cache dumps */
	struct nl_sock * sock_; 			/* pipe to RX/TX netlink messages */
	struct nl_cache * link_cache_;			/* our copy of the system link state */
	struct nl_cache * route_cache_;			/* our copy of the system route state */
	struct nl_cache_mngr * manager_;		/* wraps caches and notifies us upon a change */
	boost::ptr_list<TapIntf> interfaces_;
	boost::thread * netlink_listener_thread_;	/* polls cache manager for updates */
	boost::thread * host_forwarding_thread_;	/* polls host iface FDs for packets en route to the dataplane */

	/* no copy or assign */
	NetlinkListener(const NetlinkListener &);
	NetlinkListener& operator=(const NetlinkListener &);

	/* logging */
	void log_and_die(const char * msg);
	void log_and_die_rc(const char * msg, int rc);

	/* initialize data structures */
	void init();
	void init_dump_params();
	struct nl_dump_params * get_dump_params();

	/* virtual interfaces */
	void init_ifaces(const std::string &prefix, const int qty);
	void delete_ifaces(const std::string &prefix, const int qty);
	void init_packet_forwarding();

	/* netlink callbacks */
	static void netlink_link_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data);
	static void netlink_route_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data);

	/* cache manager polling thread */
	static void netlink_listener(int pollIntervalMillis, NetlinkListener * nll);
	static void host_packet_listener( );
};
