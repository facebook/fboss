/* C headers */
extern "C" {
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <errno.h>
}
/* C++ headers */
#include <string>
#include <sstream>
#include <iostream>

#include "fboss/agent/types.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/EthHdr.h"

namespace facebook { namespace fboss {

class TapIntf {
	public:
	TapIntf(const std::string &name, RouterID rid, InterfaceID iid);
	~TapIntf();

	inline std::string& getIfaceName() {
		return name_;
	};
	inline int getIfaceFD() {
		return fd_;
	};
	inline unsigned short getIfaceIndex() {
		return index_;
	};
	inline RouterID getIfaceRouterID() {
		return rid_;
	};
	inline InterfaceID getInterfaceID() {
		return iid_;
	};

	bool sendPacketToHost(std::unique_ptr<RxPacket> pkt);

	private:
	/* class-local variables */
	std::string name_;
	int fd_;
	unsigned short index_;
	RouterID rid_;
	InterfaceID iid_;
	//TODO std::mutex mutex_;

	int bring_up_iface();
	void take_down_iface();

	/* Disallow copy */
	TapIntf(const TapIntf &);
	TapIntf& operator=(const TapIntf &);
};

}} /* facebook::fboss */
