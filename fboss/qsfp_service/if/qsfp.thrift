namespace cpp2 facebook.fboss
namespace go neteng.fboss.qsfp
namespace py neteng.fboss.qsfp
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.qsfp

include "common/fb303/if/fb303.thrift"
include "fboss/agent/if/ctrl.thrift"
include "fboss/agent/if/fboss.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"
include "fboss/agent/switch_config.thrift"
include "fboss/mka_service/if/mka_structs.thrift"

service QsfpService extends fb303.FacebookService {
  transceiver.TransceiverType getType(1: i32 idx);

  /*
   * Get all information about a transceiver
   */
  map<i32, transceiver.TransceiverInfo> getTransceiverInfo(
    1: list<i32> idx,
  ) throws (1: fboss.FbossBaseError error);
  /*
   * Customise the transceiver based on the speed at which it should run
   */
  void customizeTransceiver(
    1: i32 idx,
    2: switch_config.PortSpeed speed,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Do a raw read on the data for a specific transceiver.
   */
  map<i32, transceiver.RawDOMData> getTransceiverRawDOMData(
    1: list<i32> idx,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Read on the raw DOM data for a specific transceiver as a union.
   */
  map<i32, transceiver.DOMDataUnion> getTransceiverDOMDataUnion(
    1: list<i32> idx,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Tell the qsfp service about the status of ports, retrieve transceiver information
   * for each of these ports.
   */
  map<i32, transceiver.TransceiverInfo> syncPorts(
    1: map<i32, ctrl.PortStatus> ports,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Qsfp service has an internal remediation loop and may potentially perform
   * interruptive operation to modules that carry no active(up) link. However
   * it may cause some confusion for debugging. This function is to tell
   * qsfp-service to pause auto remediation for the specified amount of seconds.
   */
  void pauseRemediation(1: i32 timeout);

  /*
   * Qsfp service has an internal remediation loop and may potentially perform
   * interruptive operation to modules that carry no active(up) link. However
   * it may cause some confusion for debugging. This function is to tell
   * what is the currently pause remediation expiration time set to.
   */
  i32 getRemediationUntilTime();

  /*
  * Perform a raw register read for the specified transceivers
  */
  map<i32, transceiver.ReadResponse> readTransceiverRegister(
    1: transceiver.ReadRequest request,
  ) throws (1: fboss.FbossBaseError error);

  /*
  * Perform a raw register write for the specified transceivers
  */
  map<i32, transceiver.WriteResponse> writeTransceiverRegister(
    1: transceiver.WriteRequest request,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * MACSEC capable port ids
  */
  list<i32> getMacsecCapablePorts() throws (1: fboss.FbossBaseError error);
  /*
   * Thrift call to program one PHY port. This call will be made by agent to
   * qsfp_service
   */
  void programXphyPort(
    1: i32 portId,
    2: switch_config.PortProfileID portProfileId,
  );

  bool sakInstallRx(
    1: mka_structs.MKASak sak,
    2: mka_structs.MKASci sciToAdd,
  ) throws (1: mka_structs.MKAServiceException ex) (cpp.coroutine);

  bool sakInstallTx(1: mka_structs.MKASak sak) throws (
    1: mka_structs.MKAServiceException ex,
  ) (cpp.coroutine);

  bool sakDeleteRx(
    1: mka_structs.MKASak sak,
    2: mka_structs.MKASci sciToRemove,
  ) throws (1: mka_structs.MKAServiceException ex) (cpp.coroutine);

  bool sakDelete(1: mka_structs.MKASak sak) throws (
    1: mka_structs.MKAServiceException ex,
  ) (cpp.coroutine);

  mka_structs.MKASakHealthResponse sakHealthCheck(
    1: mka_structs.MKASak sak,
  ) throws (1: mka_structs.MKAServiceException ex) (cpp.coroutine);

  mka_structs.MacsecPortPhyMap macsecGetPhyPortInfo() throws (
    1: fboss.FbossBaseError error,
  ) (cpp.coroutine);

  ctrl.PortOperState macsecGetPhyLinkInfo(1: string portName) throws (
    1: fboss.FbossBaseError error,
  ) (cpp.coroutine);

  bool deleteAllSc(1: string portName) throws (1: fboss.FbossBaseError error) (
    cpp.coroutine,
  );

  mka_structs.MacsecAllScInfo macsecGetAllScInfo(1: string portName) throws (
    1: fboss.FbossBaseError error,
  ) (cpp.coroutine);

  mka_structs.MacsecPortStats macsecGetPortStats(
    1: string portName,
    2: bool directionIngress,
    3: bool readFromHardware = false,
  ) throws (1: fboss.FbossBaseError error) (cpp.coroutine);

  mka_structs.MacsecFlowStats macsecGetFlowStats(
    1: string portName,
    2: bool directionIngress,
    3: bool readFromHardware = false,
  ) throws (1: fboss.FbossBaseError error) (cpp.coroutine);

  mka_structs.MacsecSaStats macsecGetSaStats(
    1: string portName,
    2: bool directionIngress,
    3: bool readFromHardware = false,
  ) throws (1: fboss.FbossBaseError error) (cpp.coroutine);
}
