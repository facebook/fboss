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

#define MAC_ADDRSTRLEN 18
void NetlinkListener::netlink_link_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data)
{
	struct rtnl_link * link = (struct rtnl_link *) obj;
	NetlinkListener * nll = (NetlinkListener *) data;
	const std::string name(rtnl_link_get_name(link));
	std::cout << "Link cache callback was triggered for link: " << name << std::endl;

        //nl_cache_dump(cache, nll->get_dump_params());

	/* Is this update for one of our tap interfaces? */
	const int ifindex = rtnl_link_get_ifindex(link);
	std::map<int, TapIntf *>::const_iterator itr = nll->getInterfacesByIfindex().find(ifindex);
	TapIntf * tapIface = itr->second;
	if (itr == nll->getInterfacesByIfindex().end()) /* shallow ptr cmp */
	{
		std::cout << "Ignoring netlink Link update for interface " << name << ", ifindex=" << std::to_string(ifindex) << std::endl;
		return;
	}

	/* 
	 * Things we need to consider when updating: 
	 * (1) MAC
	 * (2) MTU
	 * (3) State (UP/DOWN)
	 *
	 * We really don't care about the state, since any packets
	 * to be transmitted by the host will have no route and any
	 * packets to be received will have no active IP to go to.
	 */

	/* 
	 * We'll handle add and change for updates. del for our 
	 * taps will/should only occur when we're exiting.
	 */
	if (nl_operation == NL_ACT_DEL)
	{
		std::cout << "Ignoring netlink link remove for interface " << name << ", ifindex" << std::to_string(ifindex) << std::endl;
		return;
	}

	const std::shared_ptr<SwitchState> state = nll->sw_->getState();
	const std::shared_ptr<Interface> interface = state->getInterfaces()->getInterface(tapIface->getInterfaceID());
	
	/* Did the MAC get updated? */
	bool updateMac = false;
	char macStr[MAC_ADDRSTRLEN];
	MacAddress nlMac(nl_addr2str(rtnl_link_get_addr(link), macStr, MAC_ADDRSTRLEN));
	if (nlMac != interface->getMac())
	{
		std::cout << "Updating interface " << name << " MAC from " << interface->getMac() << " to " << nlMac << std::endl;
		updateMac = true;
	}

	/* Did the MTU get updated? */
	bool updateMtu = false;
	unsigned int nlMtu = rtnl_link_get_mtu(link);
	if (nlMtu != interface->getMtu()) /* unsigned vs signed comparison...shouldn't be an issue though since MTU typically < ~9000 */
	{
		std::cout << "Updating interface " << name << " MTU from " << std::to_string(interface->getMtu()) << " to " << std::to_string(nlMtu) << std::endl;
		updateMtu = true;
	}
	
	if (updateMac || updateMtu)
	{
		auto updateLinkFn = [=](const std::shared_ptr<SwitchState>& state)
		{
			std::shared_ptr<SwitchState> newState = state->clone();
			std::shared_ptr<Interface> newInterface = newState->getInterfaces()->getInterface(tapIface->getInterfaceID());	
			if (updateMac)
			{
				newInterface->setMac(MacAddress::fromNBO(nlMac.u64NBO()));
			}
			if (updateMtu)
			{
				newInterface->setMtu(nlMtu);
			}
			return newState;
		};

		nll->sw_->updateStateBlocking("NetlinkListener update Interface " + name, updateLinkFn);
	}
	return;
}

void NetlinkListener::netlink_route_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data)
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

	RouterID routerId;

	const struct nl_list_head * nhl = rtnl_route_get_nexthops(route);
	if (nhl == NULL || nhl->next == NULL || rtnl_route_nh_get_gateway((rtnl_nexthop *) nhl->next) == NULL) {
		std::cout << "Could not find next hop for route: " << std::endl;
		nl_object_dump((nl_object *) route, nll->get_dump_params());
		return;
	}
	else
	{
		const int ifindex = rtnl_route_nh_get_ifindex((rtnl_nexthop *) nhl->next);
		std::map<int, TapIntf *>::const_iterator itr = nll->getInterfacesByIfindex().find(ifindex);
		if (itr == nll->getInterfacesByIfindex().end()) /* shallow ptr cmp */
		{
			std::cout << "Interface index " << std::to_string(ifindex) << " not found" << std::endl;
			return;
		}
		
		routerId = itr->second->getIfaceRouterID();
		std::cout << "Interface index " << std::to_string(ifindex) << " located on RouterID " 
			<< std::to_string(routerId) << ", iface name " << itr->second->getIfaceName() << std::endl;

		struct nl_addr * addr = rtnl_route_nh_get_gateway((rtnl_nexthop *) nhl->next);
		folly::IPAddress nextHop(nl_addr2str(addr, tmp, str_len));
		fbossNextHops.emplace(nextHop);
	}
	
	switch (nl_operation)
	{
		case NL_ACT_NEW:
			{
			is_ipv4 ? nll->sw_->stats()->addRouteV4() : nll->sw_->stats()->addRouteV6();

			/* Perform the update */
			auto addFn = [=](const std::shared_ptr<SwitchState>& state)
			{
				RouteUpdater updater(state->getRouteTables());
				if (fbossNextHops.size())
				{
					updater.addRoute(routerId, network, mask, std::move(fbossNextHops));
				}
				else
				{
					updater.addRoute(routerId, network, mask, RouteForwardAction::DROP);
				}
				auto newRt = updater.updateDone();
				if (!newRt)
				{
					return std::shared_ptr<SwitchState>();
				}
				auto newState = state->clone();
				newState->resetRouteTables(std::move(newRt));
				return newState;
			};

			nll->sw_->updateStateBlocking("add route", addFn);
			}
			break; /* NL_ACT_NEW */
		case NL_ACT_DEL:
			{
			is_ipv4 ? nll->sw_->stats()->delRouteV4() : nll->sw_->stats()->delRouteV6();
			
			/* Perform the update */
			auto delFn = [=](const std::shared_ptr<SwitchState>& state) 
			{
				RouteUpdater updater(state->getRouteTables());
				updater.delRoute(routerId, network, mask);
				auto newRt = updater.updateDone();
				if (!newRt)
				{
					return std::shared_ptr<SwitchState>();
				}
				auto newState = state->clone();
				newState->resetRouteTables(std::move(newRt));
				return newState;
			};

			nll->sw_->updateStateBlocking("delete route", delFn);
			}
			break; /* NL_ACT_DEL */
		case NL_ACT_CHANGE:
			std::cout << "Not updating state due to unimplemented NL_ACT_CHANGE netlink operation" << std::endl;
			break; /* NL_ACT_CHANGE */
		default:
			std::cout << "Not updating state due to unknown netlink operation " << std::to_string(nl_operation) << std::endl;
			break; /* NL_ACT_??? */
	}
	return;
}

void NetlinkListener::netlink_neighbor_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data)
{
	struct rtnl_neigh * neigh = (struct rtnl_neigh *) obj;
	NetlinkListener * nll = (NetlinkListener *) data;
	std::cout << "Neighbor cache callback was triggered:" << std::endl;
	//nl_cache_dump(cache, nll->get_dump_params());
}

void NetlinkListener::netlink_address_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data)
{
	struct rtnl_addr * addr = (struct rtnl_addr *) obj;
	NetlinkListener * nll = (NetlinkListener *) data;
	std::cout << "Address cache callback was triggered:" << std::endl;

	/* Verify this update is for one of our taps */
	int ifindex = rtnl_addr_get_ifindex(addr);
	const std::map<int, TapIntf *>::const_iterator itr = nll->getInterfacesByIfindex().find(ifindex);
	if (itr == nll->getInterfacesByIfindex().end()) /* shallow ptr cmp */
	{
		std::cout << "Not changing IP for interface index " << std::to_string(ifindex) << " not found" << std::endl;
		return;
	}
	


	//nl_cache_dump(cache, nll->get_dump_params());
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
   		unregister_w_netlink();
    		log_and_die_rc("Connecting to netlink socket failed", rc);
  	}
  	else
  	{
		std::cout << "Connected to netlink socket" << std::endl;
  	}

  	if ((rc = rtnl_link_alloc_cache(sock_, AF_UNSPEC, &link_cache_)) < 0)
  	{
    		unregister_w_netlink();
    		log_and_die_rc("Allocating link cache failed", rc);
  	}
  	else
  	{
		std::cout << "Allocated link cache" << std::endl;
  	}

  	if ((rc = rtnl_route_alloc_cache(sock_, AF_UNSPEC, 0, &route_cache_)) < 0)
  	{
    		unregister_w_netlink();
		log_and_die_rc("Allocating route cache failed", rc);
  	}
  	else
  	{
		std::cout << "Allocated route cache" << std::endl;
  	}
	
  	if ((rc = rtnl_neigh_alloc_cache(sock_, &neigh_cache_)) < 0)
  	{
    		unregister_w_netlink();
		log_and_die_rc("Allocating neighbor cache failed", rc);
  	}
  	else
  	{
		std::cout << "Allocated neighbor cache" << std::endl;
  	}
  	
	if ((rc = rtnl_addr_alloc_cache(sock_, &addr_cache_)) < 0)
  	{
    		unregister_w_netlink();
		log_and_die_rc("Allocating address cache failed", rc);
  	}
  	else
  	{
		std::cout << "Allocated address cache" << std::endl;
  	}
	
  	if ((rc = nl_cache_mngr_alloc(NULL, AF_UNSPEC, 0, &manager_)) < 0)
  	{
    		unregister_w_netlink();
		log_and_die_rc("Failed to allocate cache manager", rc);
  	}
  	else
  	{
		std::cout << "Allocated cache manager" << std::endl;
  	}

  	nl_cache_mngt_provide(link_cache_);
	nl_cache_mngt_provide(route_cache_);
	nl_cache_mngt_provide(neigh_cache_);
	nl_cache_mngt_provide(addr_cache_);

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
    		unregister_w_netlink();
		log_and_die_rc("Failed to add route cache to cache manager", rc);
  	}
  	else
  	{
		std::cout << "Added route cache to cache manager" << std::endl;
  	}

  	if ((rc = nl_cache_mngr_add_cache(manager_, link_cache_, netlink_link_updated, this)) < 0)
  	{
    		unregister_w_netlink();
    		log_and_die_rc("Failed to add link cache to cache manager", rc);
  	}
  	else
  	{	
		std::cout << "Added link cache to cache manager" << std::endl;
  	}

  	if ((rc = nl_cache_mngr_add_cache(manager_, neigh_cache_, netlink_neighbor_updated, this)) < 0)
  	{
    		unregister_w_netlink();
    		log_and_die_rc("Failed to add neighbor cache to cache manager", rc);
  	}
  	else
  	{	
		std::cout << "Added neighbor cache to cache manager" << std::endl;
  	}

  	if ((rc = nl_cache_mngr_add_cache(manager_, addr_cache_, netlink_address_updated, this)) < 0)
  	{
    		unregister_w_netlink();
    		log_and_die_rc("Failed to add address cache to cache manager", rc);
  	}
  	else
  	{	
		std::cout << "Added address cache to cache manager" << std::endl;
  	}
}

void NetlinkListener::unregister_w_netlink()
{
	/* these will fail silently/will log error (which is okay) if it's not allocated in the first place */
	nl_cache_mngr_free(manager_);
	nl_cache_free(link_cache_);
    	nl_cache_free(route_cache_);
    	nl_cache_free(neigh_cache_);
	nl_cache_free(addr_cache_);
	nl_socket_free(sock_);
	sock_ = 0;
	std::cout << "Unregistered with netlink" << std::endl;
}

void NetlinkListener::add_ifaces(const std::string &prefix, std::shared_ptr<SwitchState> state)
{
	if (sock_ == 0)
	{
		register_w_netlink();
	}

	/* 
	 * Must update both Interfaces and Vlans, since they "reference" each other
	 * by keeping track of each others' VlanID and InterfaceID, respectively.
	 */
	std::shared_ptr<InterfaceMap> interfaces = std::make_shared<InterfaceMap>();
	std::shared_ptr<VlanMap> newVlans = std::make_shared<VlanMap>();
	for (const auto& oldVlan : state->getVlans()->getAllNodes()) /* const VlanMap::NodeContainer& vlans */
	{
		std::string interfaceName = prefix + std::to_string((int) oldVlan.second->getID()); /* stick with the original VlanIDs as given in JSON config */
		std::string vlanName = "vlan" + std::to_string((int) oldVlan.second->getID());

		std::shared_ptr<Interface> interface = std::make_shared<Interface>(
			(InterfaceID) oldVlan.second->getID(), /* TODO why not? */
			(RouterID) 0, /* TODO assume single router */
			oldVlan.second->getID(),
			interfaceName,
			MacAddress::ZERO,
			/* TODO why static_cast here? It works in ApplyThriftConfig and is static const and already initialized to 1500 in the declaration */
			static_cast<int>(Interface::kDefaultMtu) 
			);
		interfaces->addInterface(std::move(interface));

		/*
		 * Swap out the Vlan purposefully omitting any DHCP or other config.
		 * All that's needed is the VlanID, its name, and the Ports that are
		 * on that Vlan.
		 */
		std::shared_ptr<Vlan> newVlan = std::make_shared<Vlan>(
				oldVlan.second->getID(),
				vlanName
				);
		newVlan->setInterfaceID((InterfaceID) oldVlan.second->getID());	/* TODO why not? */
		newVlan->setPorts(oldVlan.second->getPorts());
		newVlans->addVlan(std::move(newVlan));
	}

	/* Update the SwitchState with the new Interfaces and Vlans */
	auto addIfacesAndVlansFn = [=](const std::shared_ptr<SwitchState>& state)
	{
		std::shared_ptr<SwitchState> newState = state->clone();	
		newState->resetIntfs(std::move(interfaces));
		newState->resetVlans(std::move(newVlans));
		return newState;
	};
	
	sw_->updateStateBlocking("Add NetlinkListener initial Interfaces and Vlans", addIfacesAndVlansFn);

	/* 
	 * Now add the tap interfaces. This will trigger lots of netlink
	 * messages that we'll handle and update our Interfaces with based on
	 * the MAC address, MTU, and (eventually) the IP address(es) detected.
	 */

	/* It's okay if we use the stale SwitchState here; only getting VlanID */
	for (const auto& vlan : state->getVlans()->getAllNodes())
	{
		std::string name = prefix + std::to_string((int) vlan.second->getID()); /* name w/same VLAN ID for transparency */

		TapIntf * tapiface = new TapIntf(
				name, 
				(RouterID) 0, /* TODO assume single router */
				(InterfaceID) vlan.second->getID() /* TODO why not? */
				); 
		interfaces_by_ifindex_[tapiface->getIfaceIndex()] = tapiface;
		interfaces_by_vlan_[vlan.second->getID()] = tapiface;
		std::cout << "Link " << name << " added." << std::endl;
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
	interfaces_by_vlan_.clear();
	std::cout << "Deleted all interfaces" << std::endl;
}

void NetlinkListener::addInterfacesAndUpdateState(std::shared_ptr<SwitchState> state)
{
	if (interfaces_by_ifindex_.size() == 0)
	{
		add_ifaces(prefix_.c_str(), state);
	}
	else
	{
		std::cout << "Not creating tap interfaces upon possible listener restart" << std::endl;
	}
}

void NetlinkListener::startNetlinkListener(const int pollIntervalMillis)
{
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

	num_ifaces = nll->getInterfacesByIfindex().size();

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

	for (std::map<int, TapIntf *>::const_iterator itr = nll->getInterfacesByIfindex().begin(), 
			end = nll->getInterfacesByIfindex().end();
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
