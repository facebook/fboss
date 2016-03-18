#include "NetlinkListener.h"

NetlinkListener::NetlinkListener(const std::string &iface_prefix, const int iface_qty): sock_(0), 
	link_cache_(0), route_cache_(0), manager_(0), 
	netlink_listener_thread_(NULL), host_packet_rx_thread_(NULL)
{
	std::cout << "Constructor of NetlinkListener" << std::endl;
	register_w_netlink();
	add_ifaces(iface_prefix.c_str(), iface_qty);
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

void NetlinkListener::log_and_die_rc(const char * msg, int rc)
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
}

void NetlinkListener::netlink_neighbor_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data)
{
	std::cout << "Neighbor cache callback triggered:" << std::endl;
	nl_cache_dump(cache, ((NetlinkListener *) data)->get_dump_params());
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

  	if ((rc = rtnl_neigh_alloc_cache(sock_, &neigh_cache_)) < 0)
  	{
    		nl_cache_free(route_cache_);
		nl_cache_free(link_cache_);
    		nl_socket_free(sock_);
    		log_and_die_rc("Allocating neighbor cache failed", rc);
  	}
  	else
  	{
		std::cout << "Allocated neighbor cache" << std::endl;
  	}

  	if ((rc = nl_cache_mngr_alloc(NULL, AF_UNSPEC, 0, &manager_)) < 0)
  	{
    		nl_cache_free(neigh_cache_);
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
	nl_cache_mngt_provide(neigh_cache_);

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

  	if ((rc = nl_cache_mngr_add_cache(manager_, neigh_cache_, netlink_neighbor_updated, this)) < 0)
  	{
    		nl_cache_mngr_free(manager_);
	    	nl_cache_free(neigh_cache_);
		nl_cache_free(link_cache_);
	    	nl_cache_free(route_cache_);
	    	nl_socket_free(sock_);
	    	log_and_die_rc("Failed to add neighbor cache to cache manager", rc);
  	}
  	else
  	{	
		std::cout << "Added neighbor cache to cache manager" << std::endl;
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

	/* (delete existing if present and) create links/ifaces */
	for (int i = 0; i < qty; i++)
	{
		int rc;
		/*struct rtnl_link * new_link = NULL;
			
		if ((new_link = rtnl_link_alloc()) == NULL)
		{
			log_and_die("Could not allocate link");
		}
		*/
		std::ostringstream iface;
		iface << prefix << i;
		std::string name = iface.str();

		/*rtnl_link_set_name(new_link, name.c_str());
		rtnl_link_set_flags(new_link, IFF_TAP | IFF_NO_PI);	
		rtnl_link_set_mtu(new_link, 1500);
		rtnl_link_set_txqlen(new_link, 1000);
		rtnl_link_set_type(new_link, "dummy");

		// delete it first, if it exists/
		if ((rc = rtnl_link_delete(sock_, new_link)) < 0)
		{
			if (rc == -NLE_NODEV)
			{
				std::cout << "Link " << name << " was not present." << std::endl;
			}
			else if (rc == -NLE_PERM || rc == -NLE_NOACCESS)
			{
				log_and_die("Insufficient permissions to add/remove tap interfaces. Perhaps you should run as sudo?");
			}
			else
			{
				std::cout << "Link " << name << " was already present. Deleted." << std::endl;
			}
		}
		if ((rc = rtnl_link_add(sock_, new_link, NLM_F_CREATE)) < 0)
		{
			if (rc == -NLE_PERM || rc == -NLE_NOACCESS)
			{
				log_and_die("Insufficient permissions to add/remove tap interfaces. Perhaps you should run as sudo?");
			}
			else 
			{
				log_and_die_rc("Unable to create interface", rc);
			}
		}
		
		// clean up 
		rtnl_link_put(new_link);
		*/
		TapIntf * tapiface = new TapIntf(name);
		interfaces_.push_back(tapiface);

		std::cout << "Link " << name << " added." << std::endl;
	}
}

void NetlinkListener::delete_ifaces()
{
	while (!interfaces_.empty())
	{
		std::cout << "Deleting interface " << interfaces_.back()->getIfaceName() << std::endl;
		delete interfaces_.back();
		interfaces_.pop_back();
	}
	std::cout << "Deleted all interfaces" << std::endl;
}

void NetlinkListener::startNetlinkListener(int pollIntervalMillis)
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

	for (std::list<TapIntf *>::const_iterator itr = nll->get_interfaces().begin(), 
			end = nll->get_interfaces().end();
			itr != end; itr++)
	{
		ev.events = EPOLLIN;
		ev.data.ptr = *itr; /* itr points to a TapIntf */
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, (*itr)->getIfaceFD(), &ev) < 0)
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

