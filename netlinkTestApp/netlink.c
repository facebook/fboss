#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <libnl3/netlink/netlink.h>
#include <libnl3/netlink/types.h>

struct nl_dump_params dump_params;


void init_dump_params()
{
	memset(&dump_params, 0, sizeof(dump_params)); /* globals should be 0ed out, but just in case we want to call this w/different params in future */
	dump_params.dp_type = NL_DUMP_STATS;
	dump_params.dp_fd = stdout;
}

struct nl_dump_params * get_dump_params()
{
	return &dump_params;
}

void log_and_die_rc(char * msg, int rc)
{
	printf("%s. RC=%d\r\n", msg, rc);
 	exit(1);
}

void log_and_die(char * msg)
{
	printf("%s\r\n", msg);
	exit(1);
}

static void link_cache_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data)
{
	struct rtnl_link * link = (struct rtnl_link *) obj;
	rtnl_link_get_name(link);
 	printf("Link cache callback was triggered for link: %s\r\n", rtnl_link_get_name(link));

        nl_cache_dump(cache, get_dump_params());
}

static void route_cache_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data)
{
	struct rtnl_route * route = (struct rtnl_route *) obj;
	printf("Route cache callback was triggered for route: %s\r\n", "");

	nl_cache_dump(cache, get_dump_params());
}

int main(void)
{
	int rc = 0; 			/* track errors; defined in libnl3/netlinks/errno.h */
	struct nl_sock * sock; 		/* pipe to RX/TX NL messages */
	struct nl_cache * link_cache;	/* our copy of the system link state */
	struct nl_cache * route_cache; 	/* our copy of the system route state */
  	struct nl_cache_mngr * manager;	/* wraps caches and notifies us where there's a change */

	init_dump_params();

	if ((sock = nl_socket_alloc()) == NULL)
	{
		log_and_die("Opening netlink socket failed");
  	}
  	else
  	{
    	printf("Opened netlink socket\r\n");
  	}
    
  	if ((rc = nl_connect(sock, NETLINK_ROUTE)) < 0)
  	{
   		nl_socket_free(sock);
    		log_and_die_rc("Connecting to netlink socket failed", rc);
  	}
  	else
  	{
  	  	printf("Connected to netlink socket\r\n");
  	}

  	if ((rc = rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache)) < 0)
  	{
    		nl_socket_free(sock);
    		log_and_die_rc("Allocating link cache failed", rc);
  	}
  	else
  	{
    		printf("Allocated link cache\r\n");
  	}

  	if ((rc = rtnl_route_alloc_cache(sock, AF_UNSPEC, 0, &route_cache)) < 0)
  	{
    		nl_cache_free(link_cache);
    		nl_socket_free(sock);
    		log_and_die_rc("Allocating route cache failed", rc);
  	}
  	else
  	{
    		printf("Allocated route cache\r\n");
  	}

  	if ((rc = nl_cache_mngr_alloc(NULL, AF_UNSPEC, 0, &manager)) < 0)
  	{
    		nl_cache_free(link_cache);
    		nl_cache_free(route_cache);
    		nl_socket_free(sock);
    		log_and_die_rc("Failed to allocate cache manager", rc);
  	}
  	else
  	{
    		printf("Allocated cache manager\r\n");
  	}

  	nl_cache_mngt_provide(link_cache);
	nl_cache_mngt_provide(route_cache);
	
	printf("Initial Cache Manager:\r\n");
  	nl_cache_mngr_info(manager, get_dump_params());
  	printf("\r\nInitial Link Cache:\r\n");
  	nl_cache_dump(link_cache, get_dump_params());
  	printf("\r\nInitial Route Cache:\r\n");
  	nl_cache_dump(route_cache, get_dump_params());

  	if ((rc = nl_cache_mngr_add_cache(manager, route_cache, route_cache_updated, NULL)) < 0)
  	{
    		nl_cache_mngr_free(manager);
    		nl_cache_free(link_cache);
    		nl_cache_free(route_cache);
    		nl_socket_free(sock);
    		log_and_die_rc("Failed to add route cache to cache manager", rc);
  	}
  	else
  	{
    		printf("Added route cache to cache manager\r\n");
  	}

  	if ((rc = nl_cache_mngr_add_cache(manager, link_cache, link_cache_updated, NULL)) < 0)
  	{
    	nl_cache_mngr_free(manager);
    	nl_cache_free(link_cache);
    	nl_cache_free(route_cache);
    	nl_socket_free(sock);
    	log_and_die_rc("Failed to add link cache to cache manager", rc);
  	}
  	else
  	{	
    	printf("Added link cache to cache manager\r\n");
  	}

  	while(1)
  	{
    		if ((rc = nl_cache_mngr_poll(manager, 5000)) < 0)
    		{
      			nl_cache_mngr_free(manager);
      			nl_cache_free(link_cache);
      			nl_cache_free(route_cache);
      			nl_socket_free(sock);
      			log_and_die_rc("Failed to set poll for cache manager", rc);
    		}
    		else
    		{
      			if (rc > 0)
      			{
				printf("Processed %d updates from netlink\r\n", rc);	
      			}
			else
			{
      				printf("No news from netlink (%d updates to process). Polling...\r\n", rc);
			}
    		}
  	}

  	return 0;
}
