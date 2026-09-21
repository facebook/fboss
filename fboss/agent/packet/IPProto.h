/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

/*
 * IPv4 and IPv6 Protocol Numbers
 *
 * References:
 *   https://en.wikipedia.org/wiki/List_of_IP_protocol_numbers
 */

namespace facebook::fboss {

enum class IP_PROTO : uint8_t {

  // RFC 2460: IPv6 Hop-by-Hop Option
  IP_PROTO_IPV6_HOPOPT = 0x00,

  // RFC 792: Internet Control Message Protocol
  IP_PROTO_ICMP = 0x01,

  // RFC 1112: Internet Group Management Protocol
  IP_PROTO_IGMP = 0x02,

  // RFC 823: Gateway-to-Gateway Protocol
  IP_PROTO_GGP = 0x03,

  // RFC 791: IPv4 Encapsulation
  IP_PROTO_IPV4 = 0x04,

  // RFC 1190: Internet Stream Protocol
  IP_PROTO_ST = 0x05,

  // RFC 793: Transmission Control Protocol
  IP_PROTO_TCP = 0x06,

  // RFC 2189: Core-based Trees
  IP_PROTO_CBT = 0x07,

  // RFC 888: Exterior Gateway Protocol
  IP_PROTO_EGP = 0x08,

  // Interior Gateway Protocol
  IP_PROTO_IGP = 0x09,

  // BBN RCC Monitoring
  IP_PROTO_BBN_RCC_MON = 0x0A,

  // RFC 741: Network Voice Protocol
  IP_PROTO_NVP2 = 0x0B,

  // Xerox PUP
  IP_PROTO_PUP = 0x0C,

  // ARGUS
  IP_PROTO_ARGUS = 0x0D,

  // EMCON
  IP_PROTO_EMCON = 0x0E,

  // IEN 158: Cross Net Debugger
  IP_PROTO_XNET = 0x0F,

  // Chaos
  IP_PROTO_CHAOS = 0x10,

  // RFC 768: User Datagram Protocol
  IP_PROTO_UDP = 0x11,

  // IEN 90: Multiplexing
  IP_PROTO_MUX = 0x12,

  // DCN Measurement Subsystems
  IP_PROTO_DCN_MEAS = 0x13,

  // RFC 869: Host Monitoring Protocol
  IP_PROTO_HMP = 0x14,

  // Packet Radio Measurement
  IP_PROTO_PRM = 0x15,

  // XEROX NS IDP
  IP_PROTO_XNS_IDP = 0x16,

  // Trunk 1
  IP_PROTO_TRUNK1 = 0x17,

  // Trunk 2
  IP_PROTO_TRUNK2 = 0x18,

  // Leaf 1
  IP_PROTO_LEAF1 = 0x19,

  // Leaf 2
  IP_PROTO_LEAF2 = 0x1A,

  // RFC 908: Reliable Datagram Protocol
  IP_PROTO_RDP = 0x1B,

  // RFC 938: Internet Reliable Transaction Protocol
  IP_PROTO_IRTP = 0x1C,

  // RFC 905: ISO Transport Protocol Class 4
  IP_PROTO_ISO_TP4 = 0x1D,

  // RFC 998: Bulk Data Transfer Protocol
  IP_PROTO_NETBLT = 0x1E,

  // MFE Network Services Protocol
  IP_PROTO_MFE_NSP = 0x1F,

  // MERIT Internodal Protocol
  IP_PROTO_MERIT_INP = 0x20,

  // RFC 4340: Datagram Congestion Control Protocol
  IP_PROTO_DCCP = 0x21,

  // Third Party Connect Protocol
  IP_PROTO_3PC = 0x22,

  // RFC 1479: Inter-Domain Policy Routing Protocol
  IP_PROTO_IDPR = 0x23,

  // Xpress Transport Protocol
  IP_PROTO_XTP = 0x24,

  // Datagram Delivery Protocol
  IP_PROTO_DDP = 0x25,

  // IDPR Control Message Transport Protocol
  IP_PROTO_IDPR_CMTP = 0x26,

  // TP++ Transport Protocol
  IP_PROTO_TPPP = 0x27,

  // IL Transport Protocol
  IP_PROTO_IL = 0x28,

  // RFC 2473: IPv6 Encapsulation
  IP_PROTO_IPV6 = 0x29,

  // RFC 1940: Source Demand Routing Protocol
  IP_PROTO_SDRP = 0x2A,

  // RFC 2460: IPv6 Routing Header
  IP_PROTO_IPV6_ROUTE = 0x2B,

  // RFC 2460: IPv6 Fragment Header
  IP_PROTO_IPV6_FRAG = 0x2C,

  // Inter-Domain Routing Protocol
  IP_PROTO_IDRP = 0x2D,

  // RFC 2205: Resource Reservation Protocol
  IP_PROTO_RSVP = 0x2E,

  // RFC 2784: Generic Routing Encapsulation
  IP_PROTO_GRE = 0x2F,

  // Mobile Host Routing Protocol
  IP_PROTO_MHRP = 0x30,

  // BNA
  IP_PROTO_BNA = 0x31,

  // RFC 4303: Encapsulating Security Payload
  IP_PROTO_ESP = 0x32,

  // RFC 4302: Authentication Header
  IP_PROTO_AH = 0x33,

  // Integrated Net Layer Security Protocol
  IP_PROTO_INLSP = 0x34,

  // SwIPe
  IP_PROTO_SWIPE = 0x35,

  // RFC 1735: NBMA Address Resolution Protocol
  IP_PROTO_NARP = 0x36,

  // RFC 2004: IP Mobility (Min Encap)
  IP_PROTO_MOBILE = 0x37,

  // Transport Layer Security Protocol (using Kryptonet key management)
  IP_PROTO_TLSP = 0x38,

  // RFC 2356: Simple Key-Management for Internet Protocol
  IP_PROTO_SKIP = 0x39,

  // RFC 4443, RFC 4884: IPv6 ICMP
  IP_PROTO_IPV6_ICMP = 0x3A,

  // RFC 2460: IPv6 No Next Header
  IP_PROTO_IPV6_NONXT = 0x3B,

  // RFC 2460: IPv6 Destination Options
  IP_PROTO_IPV6_OPTS = 0x3C,

  // Any host internal protocol
  IP_PROTO_61 = 0x3D,

  // CFTP
  IP_PROTO_CFTP = 0x3E,

  // Any local network
  IP_PROTO_63 = 0x3F,

  // SATNET and Backroom EXPAK
  IP_PROTO_SAT_EXPAK = 0x40,

  // Kryptolan
  IP_PROTO_KRYPTOLAN = 0x41,

  // MIT Remote Virtual Disk Protocol
  IP_PROTO_RVD = 0x42,

  // Internet Pluribus Packet Core
  IP_PROTO_IPPC = 0x43,

  // Any distributed file system
  IP_PROTO_68 = 0x44,

  // SATNET Monitoring
  IP_PROTO_SAT_MON = 0x45,

  // VISA Protocol
  IP_PROTO_VISA = 0x46,

  // Internet Packet Core Utility
  IP_PROTO_IPCV = 0x47,

  // Computer Protocol Network Executive
  IP_PROTO_CPNX = 0x48,

  // Computer Protocol Heart Beat
  IP_PROTO_CPHB = 0x49,

  // Wang Span Network
  IP_PROTO_WSN = 0x4A,

  // Packet Video Protocol
  IP_PROTO_PVP = 0x4B,

  // Backroom SATNET Monitoring
  IP_PROTO_BR_SAT_MON = 0x4C,

  // SUN ND PROTOCOL-Temporary
  IP_PROTO_SUN_ND = 0x4D,

  // WIDEBAND Monitoring
  IP_PROTO_WB_MON = 0x4E,

  // WIDEBAND EXPAK
  IP_PROTO_WB_EXPAK = 0x4F,

  // International Organization for Standardization Internet Protocol
  IP_PROTO_ISO_IP = 0x50,

  // RFC 1045: Versatile Message Transaction Protocol
  IP_PROTO_VMTP = 0x51,

  // RFC 1045: Secure Versatile Message Transaction Protocol
  IP_PROTO_SECURE_VMTP = 0x52,

  // VINES
  IP_PROTO_VINES = 0x53,

  // TTP
  IP_PROTO_TTP = 0x53,

  // Internet Protocol Traffic Manager
  IP_PROTO_IPTM = 0x54,

  // NSFNET-IGP
  IP_PROTO_NSFNET_IGP = 0x55,

  // Dissimilar Gateway Protocol
  IP_PROTO_DGP = 0x56,

  // TCF
  IP_PROTO_TCF = 0x57,

  // EIGRP
  IP_PROTO_EIGRP = 0x58,

  // RFC 1583: Open Shortest Path First
  IP_PROTO_OSPF = 0x59,

  // Sprite RPC Protocol
  IP_PROTO_SPRITE_RPC = 0x5A,

  // Locus Address Resolution Protocol
  IP_PROTO_LARP = 0x5B,

  // Multicast Transport Protocol
  IP_PROTO_MTP = 0x5C,

  // AX.25
  IP_PROTO_AX25 = 0x5D,

  // RFC 2003: IP-within-IP Encapsulation Protocol
  IP_PROTO_IPIP = 0x5E,

  // Mobile Internetworking Control Protocol
  IP_PROTO_MICP = 0x5F,

  // Semaphore Communications Sec. Pro
  IP_PROTO_SCC_SP = 0x60,

  // RFC 3378: Ethernet-within-IP Encapsulation
  IP_PROTO_ETHERIP = 0x61,

  // RFC 1241: Encapsulation Header
  IP_PROTO_ENCAP = 0x62,

  // Any private encryption scheme
  IP_PROTO_99 = 0x63,

  // GMTP
  IP_PROTO_GMTP = 0x64,

  // Ipsilon Flow Management Protocol
  IP_PROTO_IFMP = 0x65,

  // PNNI over IP
  IP_PROTO_PNNI = 0x66,

  // Protocol Independent Multicast
  IP_PROTO_PIM = 0x67,

  // IBM's ARIS (Aggregate Route IP Switching) Protocol
  IP_PROTO_ARIS = 0x68,

  // Space Communications Protocol Standards
  IP_PROTO_SCPS = 0x69,

  // QNX
  IP_PROTO_QNX = 0x6A,

  // Active Networks
  IP_PROTO_AN = 0x6B,

  // RFC 3173: IP Payload Compression Protocol
  IP_PROTO_IPCOMP = 0x6C,

  // Sitara Networks Protocol
  IP_PROTO_SNP = 0x6D,

  // Compaq Peer Protocol
  IP_PROTO_COMPAQ_PEER = 0x6E,

  // IPX in IP
  IP_PROTO_IPX_IN_IP = 0x6F,

  // RFC 3768: Virtual Router Redundancy Protocol, Common Address Redundancy
  // Protocol
  IP_PROTO_VRRP = 0x70,

  // RFC 3208: PGM Reliable Transport Protocol
  IP_PROTO_PGM = 0x71,

  // Any 0-hop protocol
  IP_PROTO_114 = 0x72,

  // RFC 3931: Layer Two Tunneling Protocol Version 3
  IP_PROTO_L2TP = 0x73,

  // D-II Data Exchange (DDX)
  IP_PROTO_DDX = 0x74,

  // Interactive Agent Transfer Protocol
  IP_PROTO_IATP = 0x75,

  // Schedule Transfer Protocol
  IP_PROTO_STP = 0x76,

  // SpectraLink Radio Protocol
  IP_PROTO_SRP = 0x77,

  // Universal Transport Interface Protocol
  IP_PROTO_UTI = 0x78,

  // Simple Messaging Protocol
  IP_PROTO_SMP = 0x79,

  // Simple Multicast Protocol
  IP_PROTO_SM = 0x7A,

  // Performance Transparency Protocol
  IP_PROTO_PTP = 0x7B,

  // RFC 1142, RFC 1195: Intermediate System to Intermediate System (IS-IS)
  // Protocol over IPv4
  IP_PROTO_ISIS_OVER_IPV4 = 0x7C,

  // Flexible Intra-AS Routing Environment
  IP_PROTO_FIRE = 0x7D,

  // Combat Radio Transport Protocol
  IP_PROTO_CRTP = 0x7E,

  // Combat Radio User Datagram
  IP_PROTO_CRUDP = 0x7F,

  // ITU-T Q.2111 (1999): Service-Specific Connection-Oriented Protocol in a
  // Multilink and Connectionless Environment
  IP_PROTO_SSCOPMCE = 0x80,

  IP_PROTO_IPLT = 0x81,

  // Secure Packet Shield
  IP_PROTO_SPS = 0x82,

  // Private IP Encapsulation within IP
  IP_PROTO_PIPE = 0x83,

  // Stream Control Transmission Protocol
  IP_PROTO_SCTP = 0x84,

  // Fibre Channel
  IP_PROTO_FC = 0x85,

  // RFC 3175: Reservation Protocol End-to-End Ignore
  IP_PROTO_RSVP_E2E_IGNORE = 0x86,

  // RFC 6275: Mobility Extension Header for IPv6
  IP_PROTO_MOBILITY_HEADER = 0x87,

  // RFC 3828: Lightweight User Datagram Protocol
  IP_PROTO_UDPLITE = 0x88,

  // RFC 4023: Multiprotocol Label Switching Encapsulated in IP
  IP_PROTO_MPLS_IN_IP = 0x89,

  // RFC 5498: MANET Protocols
  IP_PROTO_MANET = 0x8A,

  // RFC 5201: Host Identity Protocol
  IP_PROTO_HIP = 0x8B,

  // RFC 5533: Site Multihoming by IPv6 Intermediation
  IP_PROTO_SHIM6 = 0x8C,

  // RFC 5840: Wrapped Encapsulating Security Payload
  IP_PROTO_WESP = 0x8D,

  // RFC 5856: Robust Header Compression
  IP_PROTO_ROHC = 0x8E,

  // 0x8F-0xFC: Unassigned

  // RFC 3692: Experiments and Testing
  IP_PROTO_253 = 0xFD,
  IP_PROTO_254 = 0xFE,

  // Reserved
  IP_PROTO_255 = 0xFF,
};

} // namespace facebook::fboss
