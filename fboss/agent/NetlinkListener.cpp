#include "NetlinkListener.h"

namespace facebook { namespace fboss {

using folly::EventBase;
using folly::make_unique;

NetlinkListener::NetlinkListener(SwSwitch * sw, EventBase * evb, std::string &iface_prefix): 
	sock_(0), link_cache_(0), route_cache_(0), manager_(0), prefix_(iface_prefix),
	netlink_listener_thread_(NULL), host_packet_rx_thread_(NULL),
	sw_(sw), evb_(evb)
{
	std::cout << "Constructor of NetlinkListener" << std::endl;
	register_w_netlink();
}

NetlinkListener::~NetlinkListener()
{
	stopNetlinkListener();
	delete_ifaces();
}

void NetlinkListener::init_dump_params()
{
	memset(&dump_params_, 0, sizeof(dump_params_)); /* globals should be 0ed out, but just in case we want to call this w/different params in future */
	dump_params_.dp_type = NL_DUMP_STATS;
	dump_params_.dp_fd = stdout;
}

struct nl_dump_params * NetlinkListener::get_dump_params()
{
	return &dump_params_;
}

void NetlinkListener::log_and_die_rc(const char * msg, const int rc)
{
	std::cout << std::string(msg) << ". RC=" << std::to_string(rc) << std::endl;
 	exit(1);
}

void NetlinkListener::log_and_die(const char * msg)
{
	std::cout << std::string(msg) << std::endl;
	exit(1);
}

void NetlinkListener::netlink_link_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data)
{
	struct rtnl_link * link = (struct rtnl_link *) obj;
	NetlinkListener * nll = (NetlinkListener *) data;
	rtnl_link_get_name(link);
	std::cout << "Link cache callback was triggered for link: " << rtnl_link_get_name(link) << std::endl;

        //nl_cache_dump(cache, nll->get_dump_params());
}

void NetlinkListener::netlink_route_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data)
{
	struct rtnl_route * route = (struct rtnl_route *) obj;
	NetlinkListener * nll = (NetlinkListener *) data;
	std::cout << "Route cache callback was triggered:" << std::endl;
	//nl_cache_dump(cache, nll->get_dump_params());
	
	const uint8_t family = rtnl_route_get_family(route);
	bool is_ipv4 = true;
	switch (family)
	{
		case AF_INET:
			is_ipv4 = true;
			break;
		case AF_INET6:
			is_ipv4 = false;
			break;
		default:
			std::cout << "Unknown address family " << std::to_string(family) << std::endl;
			return;
	}

	const uint8_t str_len = is_ipv4 ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
	char tmp[str_len];
	folly::IPAddress network(nl_addr2str(rtnl_route_get_dst(route), tmp, str_len));
	const uint8_t mask = nl_addr_get_prefixlen(rtnl_route_get_dst(route));

	RouteNextHops fbossNextHops;
	fbossNextHops.reserve(1); // TODO

	const struct nl_list_head * nhl = rtnl_route_get_nexthops(route);
	if (nhl == NULL || nhl->next == NULL || rtnl_route_nh_get_gateway((rtnl_nexthop *) nhl->next) == NULL) {
		std::cout << "Could not find next hop for route: " << std::endl;
		nl_object_dump((nl_object *) route, nll->get_dump_params());
		return;
	}
	else
	{
		const int ifindex = rtnl_route_nh_get_ifindex((rtnl_nexthop *) nhl->next);
		struct nl_addr * addr = rtnl_route_nh_get_gateway((rtnl_nexthop *) nhl->next);
		folly::IPAddress nextHop(nl_addr2str(addr, tmp, str_len));
		fbossNextHops.emplace(nextHop);
	}

	is_ipv4 ? nll->sw_->stats()->addRouteV4() : nll->sw_->stats()->addRouteV6();

}

void NetlinkListener::register_w_netlink()
{
	int rc = 0; /* track errors; defined in libnl3/netlinks/errno.h */

	init_dump_params();

	if ((sock_ = nl_socket_alloc()) == NULL)
	{
		log_and_die("Opening netlink socket failed");
  	}
  	else
  	{
		std::cout << "Opened netlink socket" << std::endl;
  	}
    
  	if ((rc = nl_connect(sock_, NETLINK_ROUTE)) < 0)
  	{
   		nl_socket_free(sock_);
    		log_and_die_rc("Connecting to netlink socket failed", rc);
  	}
  	else
  	{
		std::cout << "Connected to netlink socket" << std::endl;
  	}

  	if ((rc = rtnl_link_alloc_cache(sock_, AF_UNSPEC, &link_cache_)) < 0)
  	{
    		nl_socket_free(sock_);
    		log_and_die_rc("Allocating link cache failed", rc);
  	}
  	else
  	{
		std::cout << "Allocated link cache" << std::endl;
  	}

  	if ((rc = rtnl_route_alloc_cache(sock_, AF_UNSPEC, 0, &route_cache_)) < 0)
  	{
    		nl_cache_free(link_cache_);
    		nl_socket_free(sock_);
    		log_and_die_rc("Allocating route cache failed", rc);
  	}
  	else
  	{
		std::cout << "Allocated route cache" << std::endl;
  	}

  	if ((rc = nl_cache_mngr_alloc(NULL, AF_UNSPEC, 0, &manager_)) < 0)
  	{
    		nl_cache_free(link_cache_);
    		nl_cache_free(route_cache_);
    		nl_socket_free(sock_);
    		log_and_die_rc("Failed to allocate cache manager", rc);
  	}
  	else
  	{
		std::cout << "Allocated cache manager" << std::endl;
  	}

  	nl_cache_mngt_provide(link_cache_);
	nl_cache_mngt_provide(route_cache_);
	
	/*
	printf("Initial Cache Manager:\r\n");
  	nl_cache_mngr_info(manager_, get_dump_params());
  	printf("\r\nInitial Link Cache:\r\n");
  	nl_cache_dump(link_cache_, get_dump_params());
  	printf("\r\nInitial Route Cache:\r\n");
  	nl_cache_dump(route_cache_, get_dump_params());
	*/

  	if ((rc = nl_cache_mngr_add_cache(manager_, route_cache_, netlink_route_updated, this)) < 0)
  	{
    		nl_cache_mngr_free(manager_);
    		nl_cache_free(link_cache_);
    		nl_cache_free(route_cache_);
    		nl_socket_free(sock_);
    		log_and_die_rc("Failed to add route cache to cache manager", rc);
  	}
  	else
  	{
		std::cout << "Added route cache to cache manager" << std::endl;
  	}

  	if ((rc = nl_cache_mngr_add_cache(manager_, link_cache_, netlink_link_updated, this)) < 0)
  	{
    	nl_cache_mngr_free(manager_);
    	nl_cache_free(link_cache_);
    	nl_cache_free(route_cache_);
    	nl_socket_free(sock_);
    	log_and_die_rc("Failed to add link cache to cache manager", rc);
  	}
  	else
  	{	
		std::cout << "Added link cache to cache manager" << std::endl;
  	}
}

void NetlinkListener::unregister_w_netlink()
{
	nl_cache_mngr_free(manager_);
	nl_cache_free(link_cache_);
    	nl_cache_free(route_cache_);
    	nl_socket_free(sock_);
	std::cout << "Unregistered with netlink" << std::endl;
}

void NetlinkListener::add_ifaces(const std::string &prefix, const int qty)
{
	if (sock_ == 0)
	{
		register_w_netlink();
	}

	for (int i = 0; i < qty; i++)
	{
		std::ostringstream iface;
		iface << prefix << i;

		TapIntf * tapiface = new TapIntf(iface.str(), (RouterID) i); 
		interfaces_by_ifindex_[tapiface->getIfaceIndex()] = tapiface;

		std::cout << "Link " << iface.str() << " added." << std::endl;
	}
}

void NetlinkListener::delete_ifaces()
{

	while (!interfaces_by_ifindex_.empty())
	{
		TapIntf * iface = interfaces_by_ifindex_.end()->second; 
		std::cout << "Deleting interface " << iface->getIfaceName() << std::endl;
		interfaces_by_ifindex_.erase(iface->getIfaceIndex());
		delete iface;
	}
	if (!interfaces_by_ifindex_.empty())
	{
		std::cout << "Should have no interfaces left. Bug? Clearing..." << std::endl;
		interfaces_by_ifindex_.clear();
	}
	std::cout << "Deleted all interfaces" << std::endl;
}

void NetlinkListener::startNetlinkListener(const int pollIntervalMillis, std::shared_ptr<SwitchState> swState)
{
	if (interfaces_by_ifindex_.size() == 0)
	{
		add_ifaces(prefix_.c_str(), swState->getPorts()->numPorts());
	}
	else
	{
		std::cout << "Not creating tap interfaces upon possible listener restart" << std::endl;
	}

	if (netlink_listener_thread_ == NULL)
	{
		netlink_listener_thread_ = new boost::thread(netlink_listener, pollIntervalMillis /* args to function ptr */, this);
		if (netlink_listener_thread_ == NULL)
		{
			log_and_die("Netlink listener thread creation failed");
		}
		std::cout << "Started netlink listener thread" << std::endl;
	}
	else
	{
		std::cout << "Tried to start netlink listener thread, but thread was already started" << std::endl;
	}

	if (host_packet_rx_thread_ == NULL)
	{
		host_packet_rx_thread_ = new boost::thread(host_packet_rx_listener, this);
		if (host_packet_rx_thread_ == NULL)
		{
			log_and_die("Host packet RX thread creation failed");
		}
		std::cout << "Started host packet RX thread" << std::endl;
	}
	else
	{
		std::cout << "Tried to start host packet RX thread, but thread was already started" << std::endl;
	}
}

void NetlinkListener::stopNetlinkListener()
{
	netlink_listener_thread_->interrupt();
	delete netlink_listener_thread_;
	netlink_listener_thread_ = NULL;
	std::cout << "Stopped netlink listener thread" << std::endl;

	host_packet_rx_thread_->interrupt();
	delete host_packet_rx_thread_;
	host_packet_rx_thread_ = NULL;
	std::cout << "Stopped packet RX thread" << std::endl;

	delete_ifaces();
	unregister_w_netlink();
}

void NetlinkListener::netlink_listener(const int pollIntervalMillis, NetlinkListener * nll)
{
	int rc;
  	while(1)
  	{
		boost::this_thread::interruption_point();
    		if ((rc = nl_cache_mngr_poll(nll->manager_, pollIntervalMillis)) < 0)
    		{
      			nl_cache_mngr_free(nll->manager_);
      			nl_cache_free(nll->link_cache_);
      			nl_cache_free(nll->route_cache_);
      			nl_socket_free(nll->sock_);
      			nll->log_and_die_rc("Failed to set poll for cache manager", rc);
    		}
    		else
    		{
      			if (rc > 0)
      			{
				std::cout << "Processed " << std::to_string(rc) << " updates from netlink" << std::endl;	
      			}
			else
			{
				std::cout << "No news from netlink (" << std::to_string(rc) << " updates to process). Polling..." << std::endl;
			}
    		}
  	}
}

int NetlinkListener::read_packet_from_port(NetlinkListener * nll, TapIntf * iface)
{
	unsigned char buf[BUFLEN];
	int len;

	/* read one packet per call; assumes level-triggered epoll() */
	if ((len = read(iface->getIfaceFD(), buf, BUFLEN)) < 0)
	{
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
		{
			return 0;
		}
		else
		{
			std::cout << "read() failed on iface " << iface->getIfaceName() << ", " << strerror(errno) << std::endl;
			return -1;
		}
	}
	else if (len > 0)
	{
		/* send packet to FBOSS for output on port */
		std::cout << "Got packet on iface " << iface->getIfaceName() << ". Sending to FBOSS..." << std::endl;
		return 0;
	}
	else /* len == 0 */
	{
		std::cout << "Read from iface " << iface->getIfaceName() << " returned EOF (!?) -- ignoring" << std::endl;
		return 0;
	}
}

void NetlinkListener::host_packet_rx_listener(NetlinkListener * nll)
{
	int epoll_fd;
	struct epoll_event * events;
	struct epoll_event ev;
	int num_ifaces;

	num_ifaces = nll->get_interfaces().size();

	if ((epoll_fd = epoll_create(num_ifaces)) < 0)
	{
		std::cout << "epoll_create() failed: " << strerror(errno) << std::endl;
		exit(-1);
	}

	if ((events = (struct epoll_event *) malloc(num_ifaces * sizeof(struct epoll_event))) == NULL)
	{
		std::cout << "malloc() failed: " << strerror(errno) << std::endl;
		exit(-1);
	}

	for (std::map<int, TapIntf *>::const_iterator itr = nll->get_interfaces().begin(), 
			end = nll->get_interfaces().end();
			itr != end; itr++)
	{
		ev.events = EPOLLIN;
		ev.data.ptr = itr->second; /* itr points to a TapIntf */
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, (itr->second)->getIfaceFD(), &ev) < 0)
		{
			std::cout << "epoll_ctl() failed: " << strerror(errno) << std::endl;
			exit(-1);
		}
	}

	std::cout << "Going into epoll() loop" << std::endl;

	int err = 0;
	int nfds;
	while (!err)
	{
		if ((nfds = epoll_wait(epoll_fd, events, num_ifaces, -1)) < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
			{
				continue;
			}
			else
			{
				std::cout << "epoll_wait() failed: " << strerror(errno) << std::endl;
				exit(-1);
			}
		}
		for (int i = 0; i < nfds; i++)
		{
			TapIntf * iface = (TapIntf *) events[i].data.ptr;
			std::cout << "Got packet on iface " << iface->getIfaceName() << std::endl;
			err = nll->read_packet_from_port(nll, iface);
		}
	}

	std::cout << "Exiting epoll() loop" << std::endl;
}

}} /* facebook::fboss */
