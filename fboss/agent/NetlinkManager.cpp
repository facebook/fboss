#include "fboss/agent/NetlinkManager.h"

namespace facebook { namespace fboss {

using folly::EventBase;
using folly::make_unique;

NetlinkManager::NetlinkManager(SwSwitch * sw, EventBase * evb, std::string &iface_prefix): 
	sock_(0), link_cache_(0), route_cache_(0), manager_(0), prefix_(iface_prefix),
	netlink_listener_thread_(NULL), host_packet_rx_thread_(NULL),
	sw_(sw), evb_(evb) {
	VLOG(4) << "Constructor of NetlinkManager";
	register_w_netlink();
}

NetlinkManager::~NetlinkManager() {
	stopNetlinkManager();
	delete_ifaces();
}

void NetlinkManager::init_dump_params() {
	/* globals should be 0ed out, but just in case we want to call this w/different params in future */
	memset(&dump_params_, 0, sizeof(dump_params_));
	dump_params_.dp_type = NL_DUMP_STATS;
	dump_params_.dp_fd = stdout;
}

struct nl_dump_params * NetlinkManager::get_dump_params() {
	return &dump_params_;
}

void NetlinkManager::log_and_die_rc(const char * msg, const int rc) {
	VLOG(0) << std::string(msg) << ". RC=" << std::to_string(rc);
 	exit(1);
}

void NetlinkManager::log_and_die(const char * msg) {
	VLOG(0) << std::string(msg);
	exit(1);
}

#define MAC_ADDRSTRLEN 18
void NetlinkManager::netlink_link_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data) {
	struct rtnl_link * link = (struct rtnl_link *) obj;
	NetlinkManager * nlm = (NetlinkManager *) data;
	const std::string name(rtnl_link_get_name(link));
	VLOG(3) << "Link cache callback was triggered for link: " << name;

  //nl_cache_dump(cache, nlm->get_dump_params());

	/* Is this update for one of our tap interfaces? */
	const int ifindex = rtnl_link_get_ifindex(link);
	const std::map<int, TapIntf *>::const_iterator itr = nlm->getInterfacesByIfindex().find(ifindex);
	if (itr == nlm->getInterfacesByIfindex().end()) {
		VLOG(1) << "Ignoring netlink Link update for interface " << name << ", ifindex=" << std::to_string(ifindex);
		return;
	}
	TapIntf * tapIface = itr->second;

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
	if (nl_operation == NL_ACT_DEL) {
		VLOG(1) << "Ignoring netlink link remove for interface " << name << ", ifindex" << std::to_string(ifindex);
		return;
	}

	const std::shared_ptr<SwitchState> state = nlm->sw_->getState();
	const std::shared_ptr<Interface> interface = state->getInterfaces()->getInterface(tapIface->getInterfaceID());
	
	/* Did the MAC get updated? */
	bool updateMac = false;
	char macStr[MAC_ADDRSTRLEN];
	const MacAddress nlMac(nl_addr2str(rtnl_link_get_addr(link), macStr, MAC_ADDRSTRLEN));
	if (nlMac != interface->getMac()) {
		VLOG(0) << "Updating interface " << name << " MAC from " << interface->getMac() << " to " << nlMac;
		updateMac = true;
	}

	/* Did the MTU get updated? */
	bool updateMtu = false;
	const unsigned int nlMtu = rtnl_link_get_mtu(link);
	if (nlMtu != interface->getMtu()) { /* unsigned vs signed comparison...shouldn't be an issue though since MTU typically < ~9000 */ 
		VLOG(0) << "Updating interface " << name << " MTU from " << std::to_string(interface->getMtu()) << " to " << std::to_string(nlMtu);
		updateMtu = true;
	}
	
	if (updateMac || updateMtu) {
		auto updateLinkFn = [=](const std::shared_ptr<SwitchState>& state) {
			std::shared_ptr<SwitchState> newState = state->clone();
			Interface * newInterface = state->getInterfaces()->getInterface(tapIface->getInterfaceID())->modify(&newState);	

		  if (updateMac) {
				newInterface->setMac(MacAddress::fromNBO(nlMac.u64NBO()));
			}
			if (updateMtu) {
				newInterface->setMtu(nlMtu);
			}
			return newState;
		};

		nlm->sw_->updateStateBlocking("NetlinkManager update Interface " + name, updateLinkFn);
	}
	return;
}

void NetlinkManager::netlink_route_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data) {
	struct rtnl_route * route = (struct rtnl_route *) obj;
	NetlinkManager * nlm = (NetlinkManager *) data;
	VLOG(3) << "Route cache callback was triggered";
	//nl_cache_dump(cache, nlm->get_dump_params());
	
	const uint8_t family = rtnl_route_get_family(route);
	bool is_ipv4 = true;
	switch (family) {
		case AF_INET:
			is_ipv4 = true;
			break;
		case AF_INET6:
			is_ipv4 = false;
			break;
		default:
			VLOG(0) << "Unknown address family " << std::to_string(family);
			return;
	}

	if (rtnl_route_get_type(route) != RTN_UNICAST) {
		VLOG(1) << "Ignoring non-unicast route update";
		return;
	}

	const uint8_t ipLen = is_ipv4 ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
	char ipBuf[ipLen];
	folly::IPAddress network;
	nl_addr2str(rtnl_route_get_dst(route), ipBuf, ipLen);
	try {
		network = folly::IPAddress::createNetwork(ipBuf, -1, false).first;
		//network = folly::IPAddress(ipBuf);
	} catch (const std::exception& ex) {
		VLOG(2) << "Exception caught: " << ex.what();
		VLOG(0) << "Could not parse IP '" << ipBuf << "' in route update";
		return;
	}

	const uint8_t mask = nl_addr_get_prefixlen(rtnl_route_get_dst(route));
	VLOG(3) << "Got route update of (buf= " << std::string(ipBuf) << ") " << network.str() << "/" << std::to_string(mask);

	RouteNextHops fbossNextHops;
	fbossNextHops.reserve(1); // TODO

	RouterID routerId;

	if (!rtnl_route_get_nnexthops(route)) {
		VLOG(0) << "Could not find next hop for route: ";
		nl_object_dump((nl_object *) route, nlm->get_dump_params());
		return;
	} else {
		struct rtnl_nexthop * nh = rtnl_route_nexthop_n(route, 0); /* assume single next hop */
		const int ifindex = rtnl_route_nh_get_ifindex(nh); 
		std::map<int, TapIntf *>::const_iterator itr = nlm->getInterfacesByIfindex().find(ifindex);
		if (itr == nlm->getInterfacesByIfindex().end()) { /* shallow ptr cmp */
			VLOG(1) << "Interface index " << std::to_string(ifindex) << " not found";
			return;
		}
		
		routerId = itr->second->getIfaceRouterID();
		VLOG(1) << "Interface index " << std::to_string(ifindex) << " located on RouterID " 
			<< std::to_string(routerId) << ", iface name " << itr->second->getIfaceName();

		folly::IPAddress nextHopIp;
		nl_addr2str(rtnl_route_nh_get_gateway(nh), ipBuf, ipLen);
		try {
			nextHopIp = folly::IPAddress(ipBuf);
			fbossNextHops.emplace(nextHopIp);
		} catch (const std::exception& ex) {
			if (strcmp("none", ipBuf) == 0) {
				VLOG(2) << "Exception caught: " << ex.what();
				VLOG(1) << "Got IP of 'none' in route update for " << itr->second->getIfaceName()
					<< ". Assigning LAN route " 
					<< network.str() << "/" << std::to_string(mask);
			} else {
				VLOG(0) << "Could not parse IP '" << ipBuf << "' in route update for " 
					<< itr->second->getIfaceName();
				return;
			}
		}
	}
	
	switch (nl_operation) {
		case NL_ACT_NEW:
			{
			is_ipv4 ? nlm->sw_->stats()->addRouteV4() : nlm->sw_->stats()->addRouteV6();

			/* Perform the update */
			auto addFn = [=](const std::shared_ptr<SwitchState>& state) {
				RouteUpdater updater(state->getRouteTables());
				if (fbossNextHops.size()) {
					updater.addRoute(routerId, network, mask, std::move(fbossNextHops));
				} else {
					updater.addRoute(routerId, network, mask, RouteForwardAction::TO_CPU);
				}
				auto newRt = updater.updateDone();
				if (!newRt) {
					return std::shared_ptr<SwitchState>();
				}
				auto newState = state->clone();
				newState->resetRouteTables(std::move(newRt));
				return newState;
			};

			nlm->sw_->updateStateBlocking("add route", addFn);
			}
			break; /* NL_ACT_NEW */
		case NL_ACT_DEL:
			{
			is_ipv4 ? nlm->sw_->stats()->delRouteV4() : nlm->sw_->stats()->delRouteV6();
			
			/* Perform the update */
			auto delFn = [=](const std::shared_ptr<SwitchState>& state) {
				RouteUpdater updater(state->getRouteTables());
				updater.delRoute(routerId, network, mask);
				auto newRt = updater.updateDone();
				if (!newRt) {
					return std::shared_ptr<SwitchState>();
				}
				auto newState = state->clone();
				newState->resetRouteTables(std::move(newRt));
				return newState;
			};

			nlm->sw_->updateStateBlocking("delete route", delFn);
			}
			break; /* NL_ACT_DEL */
		case NL_ACT_CHANGE:
			VLOG(2) << "Not updating state due to unimplemented NL_ACT_CHANGE netlink operation";
			break; /* NL_ACT_CHANGE */
		default:
			VLOG(0) << "Not updating state due to unknown netlink operation " << std::to_string(nl_operation);
			break; /* NL_ACT_??? */
	}
	return;
}

void NetlinkManager::netlink_neighbor_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data) {
	struct rtnl_neigh * neigh = (struct rtnl_neigh *) obj;
	NetlinkManager * nlm = (NetlinkManager *) data;
	//nl_cache_dump(cache, nlm->get_dump_params());

	const int ifindex = rtnl_neigh_get_ifindex(neigh);
	char nameTmp[IFNAMSIZ];
	rtnl_link_i2name(nlm->link_cache_, ifindex, nameTmp, IFNAMSIZ); /* must explicitly use link cache */
	const std::string name(nameTmp);
	VLOG(1) << "Neighbor cache callback was triggered for link " << name;

	const std::map<int, TapIntf *>::const_iterator itr = nlm->getInterfacesByIfindex().find(ifindex);
	if (itr == nlm->getInterfacesByIfindex().end()) { /* shallow ptr cmp */ 
		VLOG(1) << "Not updating neighbor entry for interface " << name << ", ifindex=" << std::to_string(ifindex);
		return;
	}
	TapIntf * tapIface = itr->second;
	const std::shared_ptr<SwitchState> state = nlm->sw_->getState();
	const std::shared_ptr<Interface> interface = state->getInterfaces()->getInterface(tapIface->getInterfaceID());

	bool is_ipv4 = true;
	const int family = rtnl_neigh_get_family(neigh);
	switch (family) {
		case AF_INET:
			is_ipv4 = true;
			break;
		case AF_INET6:
			is_ipv4 = false;
			break;
		default:
			VLOG(0) << "Unknown address family " << std::to_string(family);
			return;
	}

	const uint8_t ipLen = is_ipv4 ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
	char ipBuf[ipLen];
	char macBuf[MAC_ADDRSTRLEN];
	folly::IPAddress nlIpAddress;
	folly::MacAddress nlMacAddress;
	nl_addr2str(rtnl_neigh_get_dst(neigh), ipBuf, ipLen);
	nl_addr2str(rtnl_neigh_get_lladdr(neigh), macBuf, MAC_ADDRSTRLEN);
	try {
		nlIpAddress = folly::IPAddress(ipBuf);
		nlMacAddress = folly::MacAddress(macBuf);
	} catch (const std::invalid_argument& ex) {
		VLOG(0) << "Could not parse MAC '" << macBuf << "' or IP '" << ipBuf << "' in neighbor update for ifindex " << ifindex;
		return;
	}

	switch (nl_operation) {
		case NL_ACT_NEW:
		{
			if (is_ipv4) {
				/* Perform the update */
				auto addFn = [=](const std::shared_ptr<SwitchState>& state) {
					std::shared_ptr<SwitchState> newState = state->clone();
					Vlan * vlan = newState->getVlans()->getVlan(interface->getVlanID()).get();
					const PortID port = vlan->getPorts().begin()->first; /* TODO what if we have multiple ports? */
					ArpTable * arpTable = vlan->getArpTable().get();
					std::shared_ptr<ArpEntry> arpEntry = arpTable->getNodeIf(nlIpAddress.asV4());
					if (!arpEntry) {
						arpTable = arpTable->modify(&vlan, &newState);
						arpTable->addEntry(nlIpAddress.asV4(), nlMacAddress, port, interface->getID());
					} else {
						if (arpEntry->getMac() == nlMacAddress &&
							arpEntry->getPort() == port &&
							arpEntry->getIntfID() == interface->getID() &&
							!arpEntry->isPending()) {
							return std::shared_ptr<SwitchState>(); /* already there */
						}
						arpTable = arpTable->modify(&vlan, &newState);
						arpTable->addEntry(nlIpAddress.asV4(), nlMacAddress, port, interface->getID());
					}

					return newState;
				};

				nlm->sw_->updateStateBlocking("Adding new ARP entry", addFn);
			} else {
				auto addFn = [=](const std::shared_ptr<SwitchState>& state) {
					std::shared_ptr<SwitchState> newState = state->clone();
					Vlan * vlan = newState->getVlans()->getVlan(interface->getVlanID()).get();
					const PortID port = vlan->getPorts().begin()->first; /* TODO what if we have multiple ports? */
					NdpTable * ndpTable = vlan->getNdpTable().get();
					std::shared_ptr<NdpEntry> ndpEntry = ndpTable->getNodeIf(nlIpAddress.asV6());
					if (!ndpEntry) {
						ndpTable = ndpTable->modify(&vlan, &newState);
						ndpTable->addEntry(nlIpAddress.asV6(), nlMacAddress, port, interface->getID());
					} else {
						if (ndpEntry->getMac() == nlMacAddress &&
							ndpEntry->getPort() == port &&
							ndpEntry->getIntfID() == interface->getID() &&
							!ndpEntry->isPending()) {
							return std::shared_ptr<SwitchState>(); /* already there */
						}
						ndpTable = ndpTable->modify(&vlan, &newState);
						ndpTable->addEntry(nlIpAddress.asV6(), nlMacAddress, port, interface->getID());
					}

					return newState;
				};

				nlm->sw_->updateStateBlocking("Adding new NDP entry", addFn);
			}
			break; /* NL_ACL_NEW */
		}
		case NL_ACT_DEL:
		{
			if (is_ipv4) {
				/* Perform the update */
				auto delFn = [=](const std::shared_ptr<SwitchState>& state) {
					std::shared_ptr<SwitchState> newState = state->clone();
					Vlan * vlan = newState->getVlans()->getVlan(interface->getVlanID()).get();
					ArpTable * arpTable = vlan->getArpTable().get();
					std::shared_ptr<ArpEntry> arpEntry = arpTable->getNodeIf(nlIpAddress.asV4());
					if (arpEntry) {
						arpTable = arpTable->modify(&vlan, &newState);
						arpTable->removeNode(arpEntry);
					} else {
						return std::shared_ptr<SwitchState>();
					}

					return newState;
				};

				nlm->sw_->updateStateBlocking("Removing expired ARP entry", delFn);
			} else {
				/* Perform the update */
				auto delFn = [=](const std::shared_ptr<SwitchState>& state) {
					std::shared_ptr<SwitchState> newState = state->clone();
					Vlan * vlan = newState->getVlans()->getVlan(interface->getVlanID()).get();
					NdpTable * ndpTable = vlan->getNdpTable().get();
					std::shared_ptr<NdpEntry> ndpEntry = ndpTable->getNodeIf(nlIpAddress.asV6());
					if (ndpEntry) {
						ndpTable = ndpTable->modify(&vlan, &newState);
						ndpTable->removeNode(ndpEntry);
					} else {
						return std::shared_ptr<SwitchState>();
					}

					return newState;
				};

				nlm->sw_->updateStateBlocking("Removing expired NDP entry", delFn);
			}
			break; /* NL_ACT_DEL */
		}
		case NL_ACT_CHANGE:
			VLOG(1) << "Not updating state due to unimplemented NL_ACT_CHANGE netlink operation";
			break; /* NL_ACT_CHANGE */
		default:
			VLOG(0) << "Not updating state due to unknown netlink operation " << std::to_string(nl_operation);
			break; /* NL_ACT_??? */
	}
	return;
}

void NetlinkManager::netlink_address_updated(struct nl_cache * cache, struct nl_object * obj, int nl_operation, void * data) {
	struct rtnl_addr * addr = (struct rtnl_addr *) obj;
	struct rtnl_link * link = rtnl_addr_get_link(addr);
	NetlinkManager * nlm = (NetlinkManager *) data;
	//nl_cache_dump(cache, nlm->get_dump_params());

	const std::string name(rtnl_link_get_name(link));
	VLOG(1) << "Address cache callback was triggered for link " << name;

	/* Verify this update is for one of our taps */
	const int ifindex = rtnl_addr_get_ifindex(addr);
	const std::map<int, TapIntf *>::const_iterator itr = nlm->getInterfacesByIfindex().find(ifindex);
	if (itr == nlm->getInterfacesByIfindex().end()) { /* shallow ptr cmp */
		VLOG(1) << "Not changing IP for interface " << name << ", ifindex=" << std::to_string(ifindex);
		return;
	}
	TapIntf * tapIface = itr->second;
	const std::shared_ptr<SwitchState> state = nlm->sw_->getState();
	const std::shared_ptr<Interface> interface = state->getInterfaces()->getInterface(tapIface->getInterfaceID());

	bool is_ipv4 = true;
	const int family = rtnl_addr_get_family(addr);
	switch (family) {
		case AF_INET:
			is_ipv4 = true;
			break;
		case AF_INET6:
			is_ipv4 = false;
			break;
		default:
			VLOG(0)	<< "Unknown address family " << std::to_string(family);
			return;
	}

	const uint8_t str_len = is_ipv4 ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
	char tmp[str_len];
	nl_addr2str(rtnl_addr_get_local(addr), tmp, str_len);
	const folly::IPAddress nlAddress = folly::IPAddress::createNetwork(tmp, -1, false).first; /* libnl3 puts IPv4 and IPv6 addresses in local */
	VLOG(3) << "Got IP address update of " << nlAddress.str() << " for interface " << name;

	switch (nl_operation) {
		case NL_ACT_NEW:
		{
			/* Ignore if we have this IP address already */
			if (interface->hasAddress(nlAddress)) {
				VLOG(0) << "Ignoring duplicate address add of " << nlAddress.str() << " on interface " << name;
				break;
			}

			/* Perform the update */
			auto addFn = [=](const std::shared_ptr<SwitchState>& state) {
				std::shared_ptr<SwitchState> newState = state->clone();
				Interface * newInterface = state->getInterfaces()->getInterface(tapIface->getInterfaceID())->modify(&newState);	
				const Interface::Addresses oldAddresses = state->getInterfaces()->getInterface(tapIface->getInterfaceID())->getAddresses();
				Interface::Addresses newAddresses; /* boost::flat_map<IPAddress, uint8_t */
				for (auto itr = oldAddresses.begin();
					       	itr != oldAddresses.end();
						itr++) {
					newAddresses.insert(folly::IPAddress::createNetwork(itr->first.str(), -1, false)); /* std::pair<IPAddress, uint8_t> */
				}
				/* Now add our new one */
				VLOG(0) << "Adding address " << nlAddress.str() << " to interface " << name;
				newAddresses.insert(folly::IPAddress::createNetwork(nlAddress.str(), -1, false));
				newInterface->setAddresses(newAddresses);

				return newState;
			};
			
			nlm->sw_->updateStateBlocking("Adding new IP address " + nlAddress.str(), addFn);
			break; /* NL_ACT_NEW */
		}
		case NL_ACT_DEL:
		{
			/* Ignore if we don't have this IP address already */
			if (!interface->hasAddress(nlAddress)) {
				VLOG(1) << "Ignoring address delete for unknown address " << nlAddress.str() << " on interface " << name;
				break;
			}
			auto delFn = [=](const std::shared_ptr<SwitchState>& state) {
				std::shared_ptr<SwitchState> newState = state->clone();
				Interface * newInterface = state->getInterfaces()->getInterface(tapIface->getInterfaceID())->modify(&newState);	
				const Interface::Addresses oldAddresses = state->getInterfaces()->getInterface(tapIface->getInterfaceID())->getAddresses();
				Interface::Addresses newAddresses; /* boost::flat_map<IPAddress, uint8_t */
				for (auto itr = oldAddresses.begin();
					       	itr != oldAddresses.end();
						itr++) {
					if (itr->first != nlAddress) {
						newAddresses.insert(*itr);
					} else {
						VLOG(0) << "Deleting address " << nlAddress.str() << " on interface " << name;
					}
				}
				/* Replace with -1 addresses */
				newInterface->setAddresses(newAddresses);

				return newState;
			};

			nlm->sw_->updateStateBlocking("Deleting old IP address " + nlAddress.str(), delFn);
			break; /* NL_ACT_DEL */
		}
		case NL_ACT_CHANGE:
			VLOG(1) << "Not updating state due to unimplemented NL_ACT_CHANGE netlink operation";
			break; /* NL_ACT_CHANGE */
		default:
			VLOG(0) << "Not updating state due to unknown netlink operation " << std::to_string(nl_operation);
			break; /* NL_ACT_??? */
	}
	return;
}

void NetlinkManager::register_w_netlink() {
	int rc = 0; /* track errors; defined in libnl3/netlinks/errno.h */

	init_dump_params();

	if ((sock_ = nl_socket_alloc()) == NULL) {
		log_and_die("Opening netlink socket failed");
  } else {
		VLOG(1) << "Opened netlink socket";
  }
    
  if ((rc = nl_connect(sock_, NETLINK_ROUTE)) < 0) {
   	unregister_w_netlink();
    log_and_die_rc("Connecting to netlink socket failed", rc);
  } else{
		VLOG(1) << "Connected to netlink socket";
  }

  if ((rc = rtnl_link_alloc_cache(sock_, AF_UNSPEC, &link_cache_)) < 0) {
    unregister_w_netlink();
    log_and_die_rc("Allocating link cache failed", rc);
  } else {
		VLOG(1) << "Allocated link cache";
  }

  if ((rc = rtnl_route_alloc_cache(sock_, AF_UNSPEC, 0, &route_cache_)) < 0) {
    unregister_w_netlink();
		log_and_die_rc("Allocating route cache failed", rc);
  } else {
		VLOG(1) << "Allocated route cache";
  }
	
  if ((rc = rtnl_neigh_alloc_cache(sock_, &neigh_cache_)) < 0) {
    unregister_w_netlink();
		log_and_die_rc("Allocating neighbor cache failed", rc);
  } else {
		VLOG(1) << "Allocated neighbor cache";
  }
  	
	if ((rc = rtnl_addr_alloc_cache(sock_, &addr_cache_)) < 0) {
    unregister_w_netlink();
		log_and_die_rc("Allocating address cache failed", rc);
  } else {
		VLOG(1) << "Allocated address cache";
  }
	
  if ((rc = nl_cache_mngr_alloc(NULL, AF_UNSPEC, 0, &manager_)) < 0) {
    unregister_w_netlink();
		log_and_die_rc("Failed to allocate cache manager", rc);
  } else{
		VLOG(1) << "Allocated cache manager";
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

  if ((rc = nl_cache_mngr_add_cache(manager_, route_cache_, netlink_route_updated, this)) < 0) {
    unregister_w_netlink();
		log_and_die_rc("Failed to add route cache to cache manager", rc);
  } else {
		VLOG(1) << "Added route cache to cache manager";
  }

  if ((rc = nl_cache_mngr_add_cache(manager_, link_cache_, netlink_link_updated, this)) < 0) {
    unregister_w_netlink();
    log_and_die_rc("Failed to add link cache to cache manager", rc);
  } else {	
		VLOG(1) << "Added link cache to cache manager";
  }

  if ((rc = nl_cache_mngr_add_cache(manager_, neigh_cache_, netlink_neighbor_updated, this)) < 0) {
    unregister_w_netlink();
    log_and_die_rc("Failed to add neighbor cache to cache manager", rc);
  } else {	
		VLOG(1) << "Added neighbor cache to cache manager";
  }

  if ((rc = nl_cache_mngr_add_cache(manager_, addr_cache_, netlink_address_updated, this)) < 0) {
    unregister_w_netlink();
    log_and_die_rc("Failed to add address cache to cache manager", rc);
  } else {	
		VLOG(1) << "Added address cache to cache manager";
  }
}

void NetlinkManager::unregister_w_netlink() {
	/* these will fail silently/will log error (which is okay) if it's not allocated in the first place */
	nl_cache_mngr_free(manager_);
	nl_cache_free(link_cache_);
    	nl_cache_free(route_cache_);
    	nl_cache_free(neigh_cache_);
	nl_cache_free(addr_cache_);
	nl_socket_free(sock_);
	sock_ = 0;
	VLOG(0) << "Unregistered with netlink";
}

void NetlinkManager::add_ifaces(const std::string &prefix, std::shared_ptr<SwitchState> state) {
	if (sock_ == 0) {
		register_w_netlink();
	}

	/* 
	 * Must update both Interfaces and Vlans, since they "reference" each other
	 * by keeping track of each others' VlanID and InterfaceID, respectively.
	 */
	std::shared_ptr<InterfaceMap> interfaces = std::make_shared<InterfaceMap>();
	std::shared_ptr<VlanMap> newVlans = std::make_shared<VlanMap>();
	
	/* Get default VLAN to use temporarily */
	std::shared_ptr<Vlan> defaultVlan = state->getVlans()->getVlan(state->getDefaultVlan());

	VLOG(0) << "Adding " << state->getVlans()->size() << " Interfaces to FBOSS";
	for (const auto& oldVlan : state->getVlans()->getAllNodes()) { /* const VlanMap::NodeContainer& vlans */
		std::string interfaceName = prefix + std::to_string((int) oldVlan.second->getID()); /* stick with the original VlanIDs as given in JSON config */
		std::string vlanName = "vlan" + std::to_string((int) oldVlan.second->getID());

		std::shared_ptr<Interface> interface = std::make_shared<Interface>(
			(InterfaceID) oldVlan.second->getID(), /* TODO why not? */
			(RouterID) 0, /* TODO assume single router */
			oldVlan.second->getID(),
			interfaceName,
			sw_->getPlatform()->getLocalMac(), /* Temporarily use CPU MAC until netlink tells us our MAC */
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
		VLOG(4) << "Updating VLAN " << newVlan->getID() << " with new Interface ID";
	}

	/* Update the SwitchState with the new Interfaces and Vlans */
	auto addIfacesAndVlansFn = [=](const std::shared_ptr<SwitchState>& state) {
		std::shared_ptr<SwitchState> newState = state->clone();	
		newState->resetIntfs(std::move(interfaces));
		newState->resetVlans(std::move(newVlans));
		return newState;
	};

	auto clearIfacesAndVlansFn = [=](const std::shared_ptr<SwitchState>& state) {
		std::shared_ptr<SwitchState> newState = state->clone();
		std::shared_ptr<InterfaceMap> delIfaces = std::make_shared<InterfaceMap>();
		std::shared_ptr<VlanMap> delVlans = std::make_shared<VlanMap>();
		delVlans->addVlan(std::move(defaultVlan));	
		newState->resetIntfs(std::move(delIfaces));
		newState->resetVlans(std::move(delVlans));
		return newState;
	};
	
	VLOG(3) << "About to update state blocking with new VLANs";
	sw_->updateStateBlocking("Purge existing Interfaces and Vlans", clearIfacesAndVlansFn);
	sw_->updateStateBlocking("Add NetlinkManager initial Interfaces and Vlans", addIfacesAndVlansFn);

	/* 
	 * Now add the tap interfaces. This will trigger lots of netlink
	 * messages that we'll handle and update our Interfaces with based on
	 * the MAC address, MTU, and (eventually) the IP address(es) detected.
	 */

	/* It's okay if we use the stale SwitchState here; only getting VlanID */
	VLOG(0) << "Adding " << state->getVlans()->size() << " tap interfaces to host";
	for (const auto& vlan : state->getVlans()->getAllNodes()) {
		std::string name = prefix + std::to_string((int) vlan.second->getID()); /* name w/same VLAN ID for transparency */

		TapIntf * tapiface = new TapIntf(
				name, 
				(RouterID) 0, /* TODO assume single router */
				(InterfaceID) vlan.second->getID() /* TODO why not? */
				); 
		interfaces_by_ifindex_[tapiface->getIfaceIndex()] = tapiface;
		interfaces_by_vlan_[vlan.second->getID()] = tapiface;
		VLOG(4) << "Tap interface " << name << " added";
	}
}

void NetlinkManager::delete_ifaces() {

	while (!interfaces_by_ifindex_.empty()) {
		TapIntf * iface = interfaces_by_ifindex_.end()->second; 
		VLOG(4) << "Deleting interface " << iface->getIfaceName();
		interfaces_by_ifindex_.erase(iface->getIfaceIndex());
		delete iface;
	}
	if (!interfaces_by_ifindex_.empty()) {
		VLOG(0) << "Should have no interfaces left. Bug? Clearing...";
		interfaces_by_ifindex_.clear();
	}
	interfaces_by_vlan_.clear();
	VLOG(0) << "Deleted all interfaces";
}

void NetlinkManager::addInterfacesAndUpdateState(std::shared_ptr<SwitchState> state) {
	if (interfaces_by_ifindex_.size() == 0) {
		add_ifaces(prefix_.c_str(), state);
	} else {
		VLOG(0) << "Not creating tap interfaces upon possible netlink manager restart";
	}
}

void NetlinkManager::startNetlinkManager(const int pollIntervalMillis) {
	if (netlink_listener_thread_ == NULL) {
		netlink_listener_thread_ = new boost::thread(netlink_listener, pollIntervalMillis /* args to function ptr */, this);
		if (netlink_listener_thread_ == NULL) {
			log_and_die("Netlink listener thread creation failed");
		}
		VLOG(0) << "Started netlink listener thread";
	} else {
		VLOG(0) << "Tried to start netlink listener thread, but thread was already started" << std::endl;
	}

	if (host_packet_rx_thread_ == NULL) {
		host_packet_rx_thread_ = new boost::thread(host_packet_rx_listener, this);
		if (host_packet_rx_thread_ == NULL) {
			log_and_die("Host packet RX thread creation failed");
		}
		VLOG(0) << "Started host packet RX thread";
	} else {
		VLOG(0) << "Tried to start host packet RX thread, but thread was already started";
	}
}

void NetlinkManager::stopNetlinkManager() {
	netlink_listener_thread_->interrupt();
	delete netlink_listener_thread_;
	netlink_listener_thread_ = NULL;
	VLOG(0) << "Stopped netlink listener thread";

	host_packet_rx_thread_->interrupt();
	delete host_packet_rx_thread_;
	host_packet_rx_thread_ = NULL;
	VLOG(0) << "Stopped packet RX thread";

	delete_ifaces();
	unregister_w_netlink();
}

void NetlinkManager::netlink_listener(const int pollIntervalMillis, NetlinkManager * nlm) {
	int rc;
  while(1) {
		boost::this_thread::interruption_point();
    if ((rc = nl_cache_mngr_poll(nlm->manager_, pollIntervalMillis)) < 0) {
    	nl_cache_mngr_free(nlm->manager_);
      nl_cache_free(nlm->link_cache_);
      nl_cache_free(nlm->route_cache_);
      nl_socket_free(nlm->sock_);
      nlm->log_and_die_rc("Failed to set poll for cache manager", rc);
    } else {
      if (rc > 0) {
				VLOG(1) << "Processed " << std::to_string(rc) << " updates from netlink";	
      } else {
				VLOG(2) << "No news from netlink (" << std::to_string(rc) << " updates to process). Polling...";
			}
    }
  }
}

int NetlinkManager::read_packet_from_port(NetlinkManager * nlm, TapIntf * iface) {
	/* Parse into TxPacket */
	int len;
	std::unique_ptr<TxPacket> pkt;
	
	std::shared_ptr<Interface> interface = nlm->sw_->getState()->getInterfaces()->getInterfaceIf(iface->getInterfaceID());
	if (!interface) {
		VLOG(0) << "Could not find FBOSS interface ID for " << iface->getIfaceName() << ". Dropping packet from host";
		return 0; /* silently fail */
	}

	pkt = nlm->sw_->allocateL2TxPacket(interface->getMtu());
	auto buf = pkt->buf();

	/* read one packet per call; assumes level-triggered epoll() */
	if ((len = read(iface->getIfaceFD(), buf->writableTail(), buf->tailroom())) < 0) {
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
			return 0;
		} else {
			VLOG(0) << "read() failed on iface " << iface->getIfaceName() << ", " << strerror(errno);
			return -1;
		}
	} else if (len > 0 && len > buf->tailroom()) {
		VLOG(0) << "Too large packet (" << std::to_string(len) << " > " << buf->tailroom() << ") received from host. Dropping packet";
	} else if (len > 0) {
		/* Send packet to FBOSS for output on port */
		VLOG(2) << "Got packet of " << std::to_string(len) << " bytes on iface " << iface->getIfaceName() << ". Sending to FBOSS...";
		buf->append(len);
		nlm->sw_->sendL2Packet(interface->getID(), std::move(pkt));
	} else { /* len == 0 */
		VLOG(0) << "Read from iface " << iface->getIfaceName() << " returned EOF (!?) -- ignoring";
	}
	return 0;
}

void NetlinkManager::host_packet_rx_listener(NetlinkManager * nlm) {
	int epoll_fd;
	struct epoll_event * events;
	struct epoll_event ev;
	int num_ifaces;

	num_ifaces = nlm->getInterfacesByIfindex().size();

	if ((epoll_fd = epoll_create(num_ifaces)) < 0) {
		VLOG(0) << "epoll_create() failed: " << strerror(errno);
		exit(-1);
	}

	if ((events = (struct epoll_event *) malloc(num_ifaces * sizeof(struct epoll_event))) == NULL) {
		VLOG(0) << "malloc() failed: " << strerror(errno);
		exit(-1);
	}

	for (std::map<int, TapIntf *>::const_iterator itr = nlm->getInterfacesByIfindex().begin(), 
			end = nlm->getInterfacesByIfindex().end();
			itr != end; itr++) {
		ev.events = EPOLLIN;
		ev.data.ptr = itr->second; /* itr points to a TapIntf */
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, (itr->second)->getIfaceFD(), &ev) < 0) {
			VLOG(0) << "epoll_ctl() failed: " << strerror(errno);
			exit(-1);
		}
	}

	VLOG(0) << "Going into epoll() loop";

	int err = 0;
	int nfds;
	while (!err) {
		if ((nfds = epoll_wait(epoll_fd, events, num_ifaces, -1)) < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			} else {
				VLOG(0) << "epoll_wait() failed: " << strerror(errno);
				exit(-1);
			}
		}
		for (int i = 0; i < nfds; i++) {
			TapIntf * iface = (TapIntf *) events[i].data.ptr;
			VLOG(2) << "Got packet on iface " << iface->getIfaceName();
			err = nlm->read_packet_from_port(nlm, iface);
		}
	}

	VLOG(0) << "Exiting epoll() loop";
}

bool NetlinkManager::sendPacketToHost(std::unique_ptr<RxPacket> pkt) {
	const VlanID vlan = pkt->getSrcVlan();
	std::map<VlanID, TapIntf *>::const_iterator itr = interfaces_by_vlan_.find(vlan);
	if (itr == interfaces_by_vlan_.end()) {
		VLOG(4) << "Dropping packet for unknown tap interface on VLAN " << std::to_string(vlan);
		return false;
	}
	return itr->second->sendPacketToHost(std::move(pkt));
}
}} /* facebook::fboss */
