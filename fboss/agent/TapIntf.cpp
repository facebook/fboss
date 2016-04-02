#include "TapIntf.h"

namespace facebook { namespace fboss {

TapIntf::TapIntf(const std::string &iface_name, RouterID rid, InterfaceID iid) : 
	name_(iface_name), fd_(0), index_(0), rid_(rid), iid_(iid) {
	bring_up_iface();
}

TapIntf::~TapIntf() {
	take_down_iface();
}

int TapIntf::bring_up_iface() {
	int rc;
	int flags;
	struct ifreq ifr;

	/* Check if we're already running */
	if (fd_ != 0) {
		VLOG(0) << "Tried to initialize " << name_ << ", but it was already running";
		return -1;
	}

	if ((fd_ = open("/dev/net/tun", O_RDWR)) < 0) {
		VLOG(0) << "Could not open file descriptor for " << name_ << ". open() returned " << fd_;
		rc = fd_;
		fd_ = 0;
		return rc;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	size_t len = std::min(name_.size(), sizeof(ifr.ifr_name));
	memmove(ifr.ifr_name, name_.c_str(), len);

	if ((rc = ioctl(fd_, TUNSETIFF, (void *) &ifr)) < 0) {
		std::string err;
		if (errno == EBADF) {
			err = std::string("Invalid file descriptor, errno=") + std::to_string(errno);	
		} else if (errno == EFAULT) {
			err = std::string("Argument references inaccessible memory area, errno=") + std::to_string(errno);
		} else if (errno == EINVAL) {
			err = std::string("File descriptor not associated with a character special device, errno=") + std::to_string(errno);
		} else if (errno == ENOTTY) {
			err = std::string("Request invalid for object type referenced by file descriptor, errno=") + std::to_string(errno);
		} else if (errno == EPERM) {
			err = std::string("Insufficient permissions to create tap interface, errno=") + std::to_string(errno);
		} else if (errno == EBUSY) {
			err = std::string("Device is busy, errno=") + std::to_string(errno);
		} else {
			err = std::string("Unknown errno=") + std::to_string(errno);
		}
		VLOG(0) << "Could not open interface " << std::string(ifr.ifr_name) << " for packet RX/TX. ioctl() returned " << rc << ". " << err;
		close(fd_);
		fd_ = 0;
		return rc;
	}

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if ((rc = ioctl(sock, SIOCGIFINDEX, &ifr)) < 0) {
		VLOG(0) << "Could not fetch index of interface " << name_ << ", errno=" << std::to_string(errno) << ", " << strerror(errno);
		close(fd_);
		fd_ = 0;
		return rc;
	}
	index_ = ifr.ifr_ifindex;
	close(sock);

	if ((flags = fcntl(fd_, F_GETFL, 0)) < 0) {
		VLOG(0) << "Could not get flags for interface " << name_ << ". fcntl() returned " << flags;
		close(fd_);
		fd_ = 0;
		return flags;
	}

	flags |= O_NONBLOCK;
	
	if ((rc = fcntl(fd_, F_SETFL, flags)) < 0) {
		VLOG(0) << "Could not set flags (nonblocking) for interface " << name_ << ". fcntl() returned " << rc;
		close(fd_);
		fd_ = 0;
		return rc;
	}

	VLOG(1) << "Created interface " << name_ << " (index=" << std::to_string(index_) << ")"; 

	return 0;
}

void TapIntf::take_down_iface() {
	int rc;
	if (fd_ == 0) {
		VLOG(0) << "Interface " << name_ << " already removed";
	} else if ((rc = close(fd_)) < 0) {
		VLOG(0) << "Error closing file descriptor for interface " << name_ << ", " << strerror(errno);
	} else {
		VLOG(1) << "Removed interface " << name_;
		fd_ = 0;
	}
}

bool TapIntf::sendPacketToHost(std::unique_ptr<RxPacket> pkt) {
	const int l2len = EthHdr::SIZE;

	if (pkt->buf()->length() <= l2len) {
		VLOG(0) << "Received too small packet (size of " << std::to_string(pkt->buf()->length()) << " bytes) from FBOSS to host. Dropping packet";
		return false;
	}

	/* Sanity check */
	if (!fd_) {
		VLOG(0) << "File descriptor for interface " << name_ << " was not set (?!). Exiting now";
		exit(1);
	}

	int rc = 0;
	do {
		rc = write(fd_, pkt->buf()->data(), pkt->buf()->length());
	} while (rc == -1 && errno == EINTR);

	if (rc < 0) {
		VLOG(0) << "Failed to send packet to interface " << name_;
		return false;
	} else if (rc < pkt->buf()->length()) {
		VLOG(0) << "Failed to send full packet to interface " << name_ << ". Sent " << std::to_string(rc) << " bytes instead of " 
			<< std::to_string(pkt->buf()->length()) << " bytes";
		return false;
	} else {
		VLOG(1) << "Sent packet of size " << std::to_string(rc) << " bytes to interface " << name_;
		return true;
	}
	return false;
}

}} /* facebook::fboss */
