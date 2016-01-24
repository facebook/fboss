#include "TapIntf.h"

TapIntf::TapIntf(const std::string &iface_name, const unsigned short iface_index) : 
	name_(iface_name), fd_(-1), index_(iface_index)
{
}

TapIntf::~TapIntf()
{
	closeIfaceFD();
}

int TapIntf::openIfaceFD()
{
	int rc;
	int flags;
	struct ifreq ifr;

	std::ostringstream fileToOpen;
	fileToOpen << std::string("/dev/net/tun");

	if ((fd_ = open(fileToOpen.str().c_str(), O_RDWR)) < 0)
	{
		std::cout << "Could not open file descriptor for " << fileToOpen.str() << name_ << ". open() returned " << fd_ << std::endl;
		rc = fd_;
		fd_ = 0;
		return rc;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	size_t len = std::min(name_.size(), sizeof(ifr.ifr_name));
	memmove(ifr.ifr_name, name_.c_str(), len);

	if ((rc = ioctl(fd_, TUNGETIFF, (void *) &ifr)) < 0)
	{
		std::cout << "Could not open interface " << std::string(ifr.ifr_name) << " for packet RX/TX. ioctl() returned " << rc << std::endl;
		close(fd_);
		fd_ = 0;
		return rc;
	}
	
	if ((flags = fcntl(fd_, F_GETFL, 0)) < 0)
	{
		std::cout << "Could not get flags for interface " << name_ << ". fcntl() returned " << flags << std::endl;
		close(fd_);
		fd_ = 0;
		return flags;
	}

	flags |= O_NONBLOCK;
	
	if ((rc = fcntl(fd_, F_SETFL, flags)) < 0)
	{
		std::cout << "Could not set flags (nonblocking) for interface " << name_ << ". fcntl() returned " << rc << std::endl;
		close(fd_);
		fd_ = 0;
		return rc;
	}

	return 0;
}

int TapIntf::closeIfaceFD()
{
	int rc;

	if (fd_ == 0)
	{
		std::cout << "File descriptor for interface " << name_ << " already closed" << std::endl;
		return -1;
	}
	else if ((rc = close(fd_)) < 0)
	{
		std::cout << "Error closing file descriptor for interface " << name_ << std::endl;
	}
}

