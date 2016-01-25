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

class TapIntf {
	public:
	TapIntf(const std::string &name, const unsigned short index);
	~TapIntf();

	inline std::string& getIfaceName()
	{
		return name_;
	};
	inline int getIfaceFD()
	{
		return fd_;
	};
	unsigned short getIfaceIndex()
	{
		return index_;
	};

	int bringUpIface();
	int takeDownIface();

	private:
	/* class-local variables */
	std::string name_;
	int fd_;
	unsigned short index_;
	
	/* Disallow copy */
	TapIntf(const TapIntf &);
	TapIntf& operator=(const TapIntf &);
};
