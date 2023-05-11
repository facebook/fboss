/*
 * This file is used to generate class interfaces and implementations for
 * `NeighborUpdater`, `NeighborUpdaterImpl` and `NeighborUpdaterNoopImpl`.
 * Having the definition in a centralized place ensures that these classes are
 * in sync and agree on the argument types being used. This file defines a
 * couple of utility macros, followed by multiple calls to
 * `NEIGHBOR_UPDATER_METHOD()`. This macro is *not* defined in this file; it's
 * the job of the caller to define a `NEIGHBOR_UPDATER_METHOD()` first, and
 * *then* include this file.
 *
 * See https://en.wikipedia.org/wiki/X_Macro for a more detailed explanation of
 * this technique.
 */

// clang-format off

#define GET_MACRO(_IGNORED,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,NAME,...) NAME

#define ARG_LIST_0(ARG_LIST_ENTRY_CB)
#define ARG_LIST_1(ARG_LIST_ENTRY_CB, TYPE, NAME) ARG_LIST_ENTRY_CB(TYPE, NAME)
#define ARG_LIST_2(ARG_LIST_ENTRY_CB, TYPE, NAME, ...) ARG_LIST_ENTRY_CB(TYPE, NAME), ARG_LIST_1(ARG_LIST_ENTRY_CB, __VA_ARGS__)
#define ARG_LIST_3(ARG_LIST_ENTRY_CB, TYPE, NAME, ...) ARG_LIST_ENTRY_CB(TYPE, NAME), ARG_LIST_2(ARG_LIST_ENTRY_CB, __VA_ARGS__)
#define ARG_LIST_4(ARG_LIST_ENTRY_CB, TYPE, NAME, ...) ARG_LIST_ENTRY_CB(TYPE, NAME), ARG_LIST_3(ARG_LIST_ENTRY_CB, __VA_ARGS__)
#define ARG_LIST_5(ARG_LIST_ENTRY_CB, TYPE, NAME, ...) ARG_LIST_ENTRY_CB(TYPE, NAME), ARG_LIST_4(ARG_LIST_ENTRY_CB, __VA_ARGS__)
#define ARG_LIST_6(ARG_LIST_ENTRY_CB, TYPE, NAME, ...) ARG_LIST_ENTRY_CB(TYPE, NAME), ARG_LIST_5(ARG_LIST_ENTRY_CB, __VA_ARGS__)
#define ARG_LIST_7(ARG_LIST_ENTRY_CB, TYPE, NAME, ...) ARG_LIST_ENTRY_CB(TYPE, NAME), ARG_LIST_6(ARG_LIST_ENTRY_CB, __VA_ARGS__)
#define ARG_LIST_8(ARG_LIST_ENTRY_CB, TYPE, NAME, ...) ARG_LIST_ENTRY_CB(TYPE, NAME), ARG_LIST_7(ARG_LIST_ENTRY_CB, __VA_ARGS__)
#define ARG_LIST(ARG_LIST_ENTRY_CB, ...) GET_MACRO("ignored", ##__VA_ARGS__, ARG_LIST_8, _, ARG_LIST_7, _, ARG_LIST_6, _, ARG_LIST_5, _, ARG_LIST_4, _, ARG_LIST_3, _, ARG_LIST_2, _, ARG_LIST_1, _, ARG_LIST_0)(ARG_LIST_ENTRY_CB, ##__VA_ARGS__)

#if !defined(NEIGHBOR_UPDATER_METHOD_NO_ARGS)
#define NEIGHBOR_UPDATER_METHOD_NO_ARGS(VISIBILITY, NAME, RETURN_TYPE) NEIGHBOR_UPDATER_METHOD(VISIBILITY, NAME, RETURN_TYPE)
#define DEFINED_NO_ARGS_VARIANT
#endif

NEIGHBOR_UPDATER_METHOD(public, flushEntry, uint32_t, VlanID, vlan, folly::IPAddress, ip)
NEIGHBOR_UPDATER_METHOD(public, flushEntryForIntf, uint32_t, InterfaceID, intfID, folly::IPAddress, ip)

// Ndp events
NEIGHBOR_UPDATER_METHOD(public, sentNeighborSolicitation, void, VlanID, vlan, folly::IPAddressV6, ip)
NEIGHBOR_UPDATER_METHOD(public, receivedNdpMine, void, VlanID, vlan, folly::IPAddressV6, ip, folly::MacAddress, mac, PortDescriptor, port, ICMPv6Type, type, uint32_t, flags)
NEIGHBOR_UPDATER_METHOD(public, receivedNdpNotMine, void, VlanID, vlan, folly::IPAddressV6, ip, folly::MacAddress, mac, PortDescriptor, port, ICMPv6Type, type, uint32_t, flags)

// Arp events
NEIGHBOR_UPDATER_METHOD(public, sentArpRequest, void, VlanID, vlan, folly::IPAddressV4, ip)
NEIGHBOR_UPDATER_METHOD(public, receivedArpMine, void, VlanID, vlan, folly::IPAddressV4, ip, folly::MacAddress, mac, PortDescriptor, port, ArpOpCode, op)
NEIGHBOR_UPDATER_METHOD(public, receivedArpNotMine, void, VlanID, vlan, folly::IPAddressV4, ip, folly::MacAddress, mac, PortDescriptor, port, ArpOpCode, op)

NEIGHBOR_UPDATER_METHOD(public, portDown, void, PortDescriptor, port)
NEIGHBOR_UPDATER_METHOD(public, portFlushEntries, void, PortDescriptor, port)

NEIGHBOR_UPDATER_METHOD_NO_ARGS(public, getArpCacheData, std::list<ArpEntryThrift>)
NEIGHBOR_UPDATER_METHOD_NO_ARGS(public, getNdpCacheData, std::list<NdpEntryThrift>)

// State update helpers
NEIGHBOR_UPDATER_METHOD(private, vlanAdded, void, VlanID, vlanID, const std::shared_ptr<SwitchState>, state)
NEIGHBOR_UPDATER_METHOD(private, vlanDeleted, void, VlanID, vlanID)
NEIGHBOR_UPDATER_METHOD(private, vlanChanged, void, VlanID, vlanID, InterfaceID, intfID, std::string, vlanName)
NEIGHBOR_UPDATER_METHOD(private, timeoutsChanged, void, std::chrono::seconds, arpTimeout, std::chrono::seconds, ndpTimeout, std::chrono::seconds, staleEntryInterval, uint32_t, maxNeighborProbes)

// Lookup class updaters
NEIGHBOR_UPDATER_METHOD(private, updateArpEntryClassID, void, VlanID, vlan, folly::IPAddressV4, ip, std::optional<cfg::AclLookupClass>, classID);
NEIGHBOR_UPDATER_METHOD(private, updateNdpEntryClassID, void, VlanID, vlan, folly::IPAddressV6, ip, std::optional<cfg::AclLookupClass>, classID);

#undef ARG_LIST
#undef GET_MACRO
#if defined(DEFINED_NO_ARGS_VARIANT)
#undef NEIGHBOR_UPDATER_METHOD_NO_ARGS
#undef DEFINED_NO_ARGS_VARIANT
#endif
