# FBOSS SAI store library

## Introduction
fboss/agent/hw/sai/store contains a library which provides a generic,
reference counted interface for loading existing SAI objects, creating new ones,
and modifying the attributes of existing, managed, objects. SaiStore was
designed with support for "Warm Boot" very much in mind. The motivations for
the design are discussed in greater detail in the "Warm Boot Design" section.

## SaiStore
The SaiStore class itself doesn't provide much independent functionality. Its
primary role is providing access to the underlying SaiObjectStores as well as
dispatching calls to all of them.

SaiStore is exposed as a folly::Singleton, so a typical use is to grab a
`shared_ptr` to an instance and then to use the templated `get` method to
extract a particular SaiObjectStore from the internal `std::tuple` of
SaiObjectStores to do something useful with it.
e.g.,
```c++
auto& fooStore = SaiStore::getInstance()->get<SaiFooTraits>();
fooStore.setObject(...);
...
```

## SaiObjectStore
SaiObjectStore is a generic container-like template class which is templated
on the SaiObjectTraits of the object being stored. For example, SaiVlanTraits
and SaiVlanMemberTraits are used as the template parameters for two separate
SaiObjectStores. SaiObjectStore provides ref-counted access to the SaiObjects
it stores: its various APIs return `shared_ptr<SaiObject<SaiObjectTraits>>`.
Since the objects are all reference counted, SaiObjectStore does not provide
an explicit deletion mechanism.

Primarily, SaiObjectStore does provide methods for:
* loading all existing SAI objects of the Traits `ObjectType`
* ensuring an object is either created with a set of attributes, or if the
  given AdapterHostKey is already in the store, updated to have the given
  set of attributes.
as well as some set of container-like operations like getting references to
objects or iterating over them.

## SaiObject
SaiObjects are what are stored in the SaiObjectStores. SaiObject is a generic
RAII wrapper type managing the "resource" of having a live SAI object in the
SAI adapter. A valid, live, SaiObject will cache the values of the AdapterKey,
the AdapterHostKey, and the CreateAttributes of the object. It can be created by
either loading it from the SAI adapter with an AdapterKey, or by creating a
new object given an AdapterHostKey and a CreateAttributes.

Given a live SaiObject, it is also possible to provide a new set of
attributes and any attributes that are different from those stored in the
SaiObject will be modified in the adapter.

When a live SaiObject is destroyed, the underlying SAI object is removed from
the adapter. For this reason, it would be highly undesirable to have two
SaiObjects representing the same SAI object. We attempt to reduce this
possibility by removing the copy constructor from SaiObject, but it is still
possible. For instance, someone could load the same AdapterKey into two
separate SaiObjects. It won't happen through the regular use of SaiStore,
however.

## Warm Boot Design
Since SaiStore is intended for use by a system that implements Warm Boot,
it has features to support that workload.

In FBOSS, Warm Boot has some high level goals that drive the design:
* No packet loss
* Fast
* No warm boot specific code per SaiSwitch code change

The packet loss requirement is provided mostly by the underlying SAI adapter and
forwarding element's implementation of Warm Boot. As long as FBOSS doesn't
mess up and accidentally affect the state of the hardware tables, it should
not affect the flow of packets.

The third requirement, of making Warm Boot transparent to the FBOSS developer
is the most interesting. Specifically, we expect a developer to be able to
* change internal SaiSwitch Manager class data structures
* change attribute values (or even which attributes we care about)
* add/remove objects
across Warm Boots without special backwards-compatibility code. 

To emphasize what this requirement implies, consider the Warm Boot approach of
serializing the state of the SaiSwitch, then reloading that state after the
restart. If, in the process of implementing a feature or bug fix, a developer
modifies an internal data structure, then the deserialization code would need
to temporarily understand both the old and new format of the data. Otherwise,
the new code would come up and not understand the old data format we went down
with. Even worse, to ensure the ability to roll back, we would first need to
ship a version that could come up with both the old and new format, and only
then ship a version that only understood the new version.

To meet this requirement, we lean on the StateDelta abstraction. We serialize
just the SwitchState at the time we did the Warm Boot shut down, ShutdownState,
(as opposed to the whole internal SaiSwitch state) and generate a StateDelta
between that state and an empty state. Then we replay that delta as if we got
it from SwSwitch. However, we crucially need to:
* skip re-creating objects that are already programmed correctly
* modify objects that exist, but the new code wants to treat differently
* remove objects that the new code does not recreate

To acheive this, when SaiSwitch starts up during a Warm Boot, it:
* reloads the existing SAI state with `SaiStore::reload`, Per SaiObjectStore,
  this fetches all the existing AdapterKeys for the object type, then uses
  SaiApi to fetch all the attributes this version of FBOSS knows about, and
  then uses the AdapterKey and CreateAttributes to create the appropriate
  AdapterHostKey. The object is stored in the RefMap by AdapterHostKey.
  Finally, a `shared_ptr` to the object is kept for the rest of the warm boot
  to ensure that it survives until it is referenced by replay.
* replays the ShutdownState StateDelta. SaiSwitch is written on top of SaiStore
  such that it uses setObject() to create/modify SAI objects. The result is
  that if an object already exists after reload, it is either referenced with
  no modification, or modified. An important point here is that SaiSwitch must
  be oriented around AdapterHostKeys.
* clears the `shared_ptr`s accumulated during "reload". Everything that was
  actually touched during "replay" will still be referenced by SaiSwitch, but
  the objects that were never touched will go to zero references and be
  removed from the underlying SAI adapter, as expected.

Hopefully, this description helps motivate and explain the slightly strange API
of using RefMap and setObject rather than a more natural CRUD API.

Note that one cool feature of this design is that if SaiSwitch always uses
setObject (even when it intends to modify an existing object) then the same
generic code handles modifying an object due to SwSwitch driven changes and
SaiSwitch code changes resulting in modification at Warm Boot.
