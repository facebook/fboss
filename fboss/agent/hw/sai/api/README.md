# FBOSS SAI api library

## Introduction
fboss/agent/hw/sai/api/ contains a library which takes advantage of the
regularity of the SAI interface to create a more declarative, safe C++ wrapper
for developing SAI applications.

The main problem it intends to solve is encoding implicit, solely documented
relationships between SAI entities into explicit, type
checked relationships. This includes things like connecting SAI attributes
to their data types, and enforcing whether or not attributes are required for
object creation.

This document will introduce the structure of the SAI interface, then explain
the design and implementation of the library on top of that interface.

## SAI overview
### apis
SAI is organized into a collection of "api"s. Each api is accessed by calling
`sai_api_query()` with the appropriate enum value denoting that api. This gives
the caller access to a struct of function pointers for manipulating the SAI
objects of the api. Examples of apis are: port, vlan, neighbor, route, and many
more. An exhaustive list can be found at
https://github.com/opencomputeproject/SAI/tree/master/inc with each header file
corresponding to an api. A typical api will look roughly like (slight paraphrase
to gloss over name differences between apis)
```c
struct sai_foo_api_t {
  create_foo = create_fn;
  remove_foo = remove_fn;
  get_foo_attribute = get_attribute_fn;
  set_foo_attribute = set_attribute_fn;
};
```

Each SAI api shares a regular design. To wit, the apis always deal with objects
which have a variety of attributes. In some apis, like VLAN, or NextHopGroup,
the objects can contain member objects, which in turn have their own attributes.
The main forms of interaction are creating objects with a list of attributes,
modifying the attributes of an existing object, or deleting an object.
Most objects have a 64 bit id with typedefed type `sai_object_id_t`.
These ids are created by the SAI implementation on object creation and
returned to the caller.

For example, an application can create a port through the port api. That requires
some mandatory attributes like speed and lane mapping, and returns a sai object
id for the port. Subsequently, the application can refer to the port by its
object id to get and set attributes like admin state or FEC to manage the port.

Some objects, like routes and neighbors, can be more numerous, and thus
are managed with an entry struct containing identifying information like
ip addresses and prefixes rather than by a `sai_object_id_t`.

### attributes
To manipulate attributes, SAI users go through a uniform interface. All
attributes are expressed via the tagged union `sai_attribute_t`:
```c
typedef struct _sai_attribute_t
{
    sai_attr_id_t id;
    sai_attribute_value_t value;
} sai_attribute_t;
```
id is an api-specific enum denoting which attribute is being used. value is a
union of the possible stored data types. Each attribute has an associated
data type, so the SAI implementation will use the appropriate member of the
`value` union, given the attribute in `id`.

A snippet from `sai_attribute_value_t` for illustrative purposes:
```c
typedef union _sai_attribute_value_t
{
  bool booldata;
  char chardata[32];
  sai_uint8_t u8;
  sai_int8_t s8;
  sai_uint16_t u16;
  sai_int16_t s16;
  sai_uint32_t u32;
  sai_int32_t s32;
  sai_uint64_t u64;
  sai_int64_t s64;
  sai_pointer_t ptr;
  sai_mac_t mac;
  sai_ip4_t ip4;
  sai_ip6_t ip6;
  sai_ip_address_t ipaddr;
  sai_ip_prefix_t ipprefix;
  sai_object_id_t oid;
  sai_object_list_t objlist;
  .
  .
  .
} sai_attribute_value_t;
```

## code samples
A few examples of code using raw SAI should help make the structure clear.
Code for error handling is present, but a little cartoonish.

In this first example, we will create a 25G port using hw lane 0 (lane mappings
are platform specific) and then read in and set its admin state to true
```c
sai_object_id_t switch_id = 0;
sai_port_api_t* port_api;
sai_status_t status;
status = sai_api_query(SAI_API_PORT, (void**)&port_api);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
sai_object_id_t port_id;
sai_attribute_t attrs[2];
attrs[0].id = SAI_PORT_ATTR_SPEED;
attrs[0].value.u32 = 25000;
attrs[1].id = SAI_PORT_ATTR_LANE_LIST;
attrs[1].value.u32list.count = 1;
*attrs[1].value.u32list.list = 0;
status = port_api->create_port(&port_id, switch_id, 2, attrs);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}

sai_attribute_t admin_state_attr;
admin_state_attr.id = SAI_PORT_ATTR_ADMIN_STATE;
status = port_api->get_port_attribute(port_id, 1, &admin_state_attr);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
bool admin_state = admin_state_attr.value.booldata;

admin_state_attr.id = SAI_PORT_ATTR_ADMIN_STATE;
admin_state_attr.booldata = true;
status = port_api->set_port_attribute(port_id, 1, &admin_state_attr);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
```

In this next example, we will demonstrate a SAI api with the member concept by
creating a next hop group (ECMP) and adding a few next hops to it.
```c
sai_object_id_t switch_id = 0;
sai_next_hop_api_t* next_hop_api;
sai_next_hop_group_api_t* next_hop_group_api;
sai_status_t status;
status = sai_api_query(SAI_API_NEXT_HOP, (void**)&next_hop_api);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
status = sai_api_query(SAI_API_NEXT_HOP_GROUP, (void**)&next_hop_group_api);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
sai_object_id_t next_hop_id1;
sai_object_id_t next_hop_id2;
sai_attribute_t attrs[3];
attrs[0].id = SAI_NEXT_HOP_ATTR_TYPE;
attrs[0].value.s32 = SAI_NEXT_HOP_TYPE_IP;
attrs[1].id = SAI_NEXT_HOP_ATTR_IP;
attrs[1].value.ipaddr.addr.ip4 = 0; // 32 bit int representation for 0.0.0.0
attrs[2].id = SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID;
// imagine we have created a router interface with id rif_id1
attrs[2].value.oid = rif_id1;
status = next_hop_api->create_next_hop(&next_hop_id1, switch_id, 3, attrs);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
// Likewise, assume ip2, sz2, and rif_id2 are the appropriate ip, rif for the
// second next hop
attrs[1].value.ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
attrs[1].value.ipaddr.addr.ip4 = htonl(1); // 32 bit int representation for 0.0.0.1
attrs[2].value.oid = rif_id2;
status = next_hop_api->create_next_hop(&next_hop_id2, switch_id, 3, attrs);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
sai_object_id_t next_hop_group_id;
sai_attribute_t next_hop_group_type_attr;
next_hop_group_type_attr.id = SAI_NEXT_HOP_GROUP_ATTR_TYPE;
next_hop_group_type.attr.value.s32 = SAI_NEXT_HOP_GROUP_TYPE_ECMP;
status = next_hop_group_api->create_next_hop_group(
  &next_hop_group_id, switch_id, 1, &next_hop_group_type_attr);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
sai_object_id_t next_hop_group_member_id1;
sai_object_id_t next_hop_group_member_id2;
sai_attribute_t group_member_attrs[2];
group_member_attrs[0].id = SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_GROUP_ID;
group_member_attrs[0].value.oid = next_hop_group_id;
group_member_attrs[1].id = SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_ID;
group_member_attrs[1].value.oid = next_hop_id1;
status = next_hop_group_api->create_next_hop_group_member(
  &next_hop_group_member_id1, switch_id, 2, group_member_attrs);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
group_member_attrs[1].value.oid = next_hop_id2;
status = next_hop_group_api->create_next_hop_group_member(
  &next_hop_group_member_id2, switch_id, 2, group_member_attrs);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
```

In this final example, we will use the neighbor api to demonstrate entry based
apis to contrast with the previous two id based apis.
```c
sai_object_id_t switch_id = 0;
sai_object_id_t rif_id; // from creating the rif
sai_neighbor_api_t* neighbor_api;
sai_status_t status;
status = sai_api_query(SAI_API_NEIGHBOR, (void**)&neighbor_api);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
sai_neighbor_entry_t neighbor_entry;
neighbor_entry.switch_id = switch_id;
neighbor_entry.rif_id = rif_id;
neighbor_entry.ip_address.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
neighbor_entry.ip_address.addr.ip4 = htonl(1); // 0.0.0.1
sai_attribute_t dst_mac_attr;
dst_mac_attr.id = SAI_NEIGHBOR_ATTR_DST_MAC;
// fill in dst_mac_attr.value.mac with data representing the mac;
status = neighbor_api->create_neighbor(neighbor_entry, 1, &dst_mac_attr);
if (status != SAI_STATUS_SUCCESS) {
  exit(status);
}
```

## Code Samples
In this section, we will re-write the three examples from the raw SAI examples
above using the library to demonstrate how it can be of help. The library
constructs are further explained in the "Design" section that follows.

Create and modify a port:
```c++
SwitchSaiId switchId{0};
SaiPortApi portApi;
std::vector<uint32_t> lanes{0};
// create a port
PortSaiId portId = portApi.create<SaiPortTraits>({lanes, 25000}, switchId);
bool adminState = portApi.getAttribute(
    portId,
    SaiPortTraits::Attributes::AdminState()); // get its admin state
SaiPortTraits::Attributes::AdminState adminStateAttribute{true};
portApi.setAttribute(portId, adminStateAttribute); // set admin state to true
```

Create and populate a next hop group:
```c++
SwitchSaiId switchId{0};
SaiNextHopApi nextHopApi;
SaiNextHopGroupApi nextHopGroupApi;
NextHopSaiId nextHopId1 = nextHopApi.create<SaiNextHopTraits>(
    {SAI_NEXT_HOP_TYPE_IP, rifId1, folly::IPAddress("0.0.0.0")},
    switchId);
NextHopSaiId nextHopId2 = nextHopApi.create<SaiNextHopTraits>(
    {SAI_NEXT_HOP_TYPE_IP, rifId2, folly::IPAddress("0.0.0.1")},
    switchId);
NextHopGroupSaiId nextHopGroupId =
    nextHopGroupApi.create<SaiNextHopGroupTraits>(
        {SAI_NEXT_HOP_GROUP_TYPE_ECMP},
        switchId);
nextHopGroupApi.create<SaiNextHopGroupMemeberTraits>(
    {nextHopGroupId, nextHopId1},
    switchId);
nextHopGroupApi.create<SaiNextHopGroupMemberTraits>(
    {nextHopGroupId, nextHopId2},
    switchId);
```

Create a neighbor:
```c++
SwitchSaiId switchId{0};
RouterInterfaceSaiId rifId; // from elsewhere
SaiNeighborApi neighborApi;
NeighborApiTypes::EntryType entry(switchId, rifId, folly::IPAddress("0.0.0.1"));
neighborApi.create<SaiNeighborTraits>(
    entry,
    {folly::MacAddress("00:00:00:00:00:01")},
    switchId);
```

## Design
The api library has two main components: SaiApis and SaiAttributes, which wrap
the corresponding raw SAI concepts.
Developers create one of each SaiApi, then create and pass in the associated
SaiAttributes to do get and set operations as in raw SAI. For create, each
SaiApi defines a `CreateAttributes` (conventionally, an std::tuple of SaiAttributes)
which will encode the supported and mandatory attributes for object creation.

SaiApiTable conveniently creates all the sai apis and provides read-only
and read-write references to them.

### SaiApi
SaiApi makes generic the code which calls `sai_api_query` for getting the api
object, as well as the code for taking SaiAttributes and making the appropriate
SAI api function calls with the wrapped `sai_attribute_t`. SaiApi exposes:
  * create
  * remove
  * getAttribute
  * setAttribute
which wrap their natural counterparts that expect `sai_attribute_t` from raw
SAI with functions that expect `SaiAttribute`.

The SaiApis are implemented with the CRTP pattern for static
polymorphism. The base class is SaiApi and is parameterized by the specific
api class, like PortApi, RouteApi, etc...

For example:
```c++
class RouteApi : public SaiApi<RouteApi> { /* ... */
};
```
Each SaiApi class can manage one or more (for example, in the case of member
objects managed by the same API) types of SAI objects. For each such
object, we define a 'Traits' class which contains critical information like
a nested Attributes class which contains aliases for the supported SaiAttributes.
The Traits class also conatins and an alias called CreateAttributes
which defines the tuple required for creating that type of object with the api.

### SaiAttribute
SaiAttribute makes generic the code which extracts the appropriate value
from the `sai_attribute_value_t` union. Let's lift a part of one of the code
samples above, and analyze it a little:
```c
next_hop_group_type_attr.id = SAI_NEXT_HOP_GROUP_ATTR_TYPE;
next_hop_group_type.attr.value.s32 = SAI_NEXT_HOP_GROUP_TYPE_ECMP;
```
In writing this, the developer has to know that the `TYPE` attribute will
be stored in an enum, which is represented as a 32 bit signed integer in
SAI, so you need to store it in the `s32` member of the union. The same sort
of coupling exists when extracting values after a get call. SaiAttribute ties
these two concepts together inextricably. The C++ code for instantiating the
SaiAttribute class for `SAI_NEXT_HOP_GROUP_ATTR_TYPE` and naming it `Type`
in NextHopGroupApiParameters::Attributes is:
```c++
struct NextHopGroupApiParameters {
  struct Attributes {
    using Type = SaiAttribute<
      sai_next_hop_group_attr_t, // type of the next hop group attribute enum
      SAI_NEXT_HOP_GROUP_ATTR_TYPE, // enum value for this attribute
      sai_int32_t>; // the data type used by the attribute
  };
};
```
Subsequently, we can get it using a NextHopGroupApi with code like:
```c++
NextHopGroupApi nextHopGroupApi; // corresponds to sai_api_query code
NextHopGroupApiParameters::Attributes::Type t;
sai_object_id_t saiId; // needs to be set to a correct id, of course
auto type = nextHopGroupApi.getAttribute(t, saiId); // actual "get_attribute"
```
and we can set it with:
```c++
NextHopGroupApiParameters::Attributes::Type t(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
nextHopGroupApi.setAttribute(t, saiId);
```
Note that in both of those examples, we did not have to know that `s32` was
the right union member for `SAI_NEXT_GROUP_TYPE_ECMP`, so it was impossible
to access the wrong member.

### SaiObjectTraits
As described in the SaiApi section above, each SaiApi manages one or more
SAI object types. For example, PortApi manages ports, and VlanApi manages vlans
and vlan members. Each SAI object must have a corresponding 'Traits' class
which is conventionally named SaiObjectTraits.
e.g., SaiPortTraits, SaiVlanTraits, SaiVlanMemberTraits

Each Traits objects models an informal SaiObjectTraits Concept. It must
provide:
* `ObjectType`: The `sai_object_type_t` enum value defined for the object.
* `SaiApiT`: The type of the SaiApi that manages this object.
* `Attributes`: A nested struct containing SaiAttribute aliases
* `AdapterKey`: The key type inside the sai adapter (typically, a
                strong typedef around `sai_object_id_t` or an entry struct).
                This is a representation of the type used to identify the
                object when interacting with the real C SAI API.
* `AdapterHostKey`: The key type used by the sai adapter host when
                    programming the object. The requirements on this type are
                    driven by the implementation of SaiStore (for full details,
                    see fboss/agent/hw/sai/store/README.md). At a high level,
                    AdapterHostKey must be immutable, uniquely identify the SAI
                    object, and be computable from a combination of the
                    AdapterKey and CreateAttributes of the SaiObject. Most
                    often, this will be a critical, identifying subset of the
                    attributes of the object.
* `CreateAttribute`: The tuple of attributes which can be set during
                     object creation. We use std::optional members to represent
                     optional (not MANDATORY_ON_CREATE) attributes.

e.g.,
```c++
struct SaiVlanTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_VLAN;
  using SaiApiT = VlanApi;
  struct Attributes {
    using EnumType = sai_vlan_attr_t;
    using VlanId = SaiAttribute<EnumType, SAI_VLAN_ATTR_VLAN_ID, sai_uint16_t>;
    using MemberList = SaiAttribute<
        EnumType,
        SAI_VLAN_ATTR_MEMBER_LIST,
        std::vector<sai_object_id_t>>;
  };

  using AdapterKey = VlanSaiId;
  using AdapterHostKey = Attributes::VlanId;
  using CreateAttributes = std::tuple<Attributes::VlanId>;
};
```

### Composite SaiAttributes
When creating SAI objects, some attributes are mandatory, some
can be set, and others are read only and can not be set at all. We
express this relationship and others by composing SaiAttributes with
std::optionals and std::tuples.

A recursive function template called `saiAttrs` can ultimately yield a pointer
to one or more `sai_attribute_t`s to feed into SAI apis from these structures.
This makes creating SAI objects less error-prone as you must create each
mandatory attribute or else the program will not compile. Conversely, there is
no place for illegal or ignored attributes during object creation.

More specifically, each Api's Types class defines a CreateAttributes which
is an alias to an std::tuple consisting of the mandatory SaiAttributes and
std::optionals of the optional SaiAttributes.

For example, to create a route, we must pass in a packet action attribute,
which can be set to mean forward, drop, or trap. If it is forward, we
also need a next hop id attribute, but not if the action is trap or drop.
For that reason, the next hop id attribute is optional in the CreateAttributes
tuple:
```c++
struct RouteApiParameters {
  struct Attributes {
    using PacketAction = SaiAttribute<...>;
    using NextHopId = SaiAttribute<...>;
  };
  using CreateAttributes = std::
      tuple<Attributes::PacketAction, std::optional<Attributes::NextHopId>>;
};
```

The create() method of the SaiApis takes the corresponding CreateAttributes
and passes an array of `sai_attribute_t` down to raw SAI.

To return to studying the next hop group api from the example,
we can create a next hop group like so :

```c++
NextHopGroupApi nextHopGroupApi;
SaiNextHopGroupTraits::Attributes::Type t(SAI_NEXT_HOP_GROUP_TYPE_ECMP);
SaiNextHopGroupTraits::Attributes::CreateAttributes c(t);
NextHopSaiId nextHopId =
    nextHopGroupApi.create<SaiNextHopGroupTraits>(c, switch_id);
```

SaiAttribute and SaiAttributeDataTypes have helpful implicit constructors,
which allows us to shorten the above code for convenience:

```c++
auto nextHopId =
  nextHopGroupApi.create<SaiNextHopGroupTraits>(
    {SAI_NEXT_HOP_GROUP_TYPE_ECMP}, switch_id);
```
