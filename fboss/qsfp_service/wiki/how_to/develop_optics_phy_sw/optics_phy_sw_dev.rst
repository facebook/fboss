.. fbmeta::
    hide_page_title=true

FBOSS Optics & Phy SW Development
##################################

Following is a description of how to build and test various Optics & Phy SW.
Note this description only illustrates basic commands.
Refer to other instructions for other commands and their options.

Devserver Setup
----------------------------

`Here is everything you need to know about using devservers at Meta <https://www.internalfb.com/intern/wiki/Devservers/>`_.
It is recommended to reserve a devserver with a larger disk space to store SW such as third party Broadcom source code.
`General development environment setup wikis are here <https://www.internalfb.com/intern/wiki/Development_Environment/>`_.
`Instructions to configure aliases for devservers you commonly connect to are here <https://www.internalfb.com/intern/wiki/Development_Environment/SSH_Config/>`_.
The instructions below are used on a devserver.

SW Build
-----------------------------

The FBOSS source code resides under the fbsource/fbcode/neteng/fboss/ and fbsource/fbcode/fboss/ directories.
To figure out a build target of a SW component, look at its TARGETS file.

QSFP service
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* To build QSFP service:
::

    % cd fbsource/fbcode
    % buck build @mode/opt --show-output //fboss/qsfp_service:qsfp_service

    If the build is successful, there will be an output similar to the one below:

    //fboss/qsfp_service:qsfp_service buck-out/gen/aab7ed39/fboss/qsfp_service/qsfp_service

* To reduce the output binary size:
::

    % strip -g buck-out/gen/aab7ed39/fboss/qsfp_service/qsfp_service

* To reserve a lab switch:

  * `Basset device finder UI <https://www.internalfb.com/netcastle/device?pool=fboss.qsfp.autotest>`_:
  Select pool fboss.qsfp.autotest and attribute transceivers_connected and reserve a free switch.

  * Command line if knowing a free switch (e.g. darwin:darwin.35.01.mpk17):
  ::

    % basset list fboss:darwin:darwin.35.01.mpk17
    % basset reserve -f fboss:darwin:darwin.35.01.mpk17 --purpose "triage" --expire "1 day"

* To run QSFP service on a lab switch, copy the output binary to this switch, and run it:
::

    % rsync buck-out/gen/aab7ed39/fboss/qsfp_service/qsfp_service root@elbert.03.09.fre101:/tmp/
    % ./qsfp_service --thrift_ssl_policy=permitted --config <path_to_agent_config> --qsfp_config <path_to_qsfp_config> &> qsfp.log &

    By default, qsfp_service looks for configuation files in the /etc/coop/ directory, which is present on production switches but not likely on lab switches.
    In this case, you need to explicity specify their locations:

    % ./qsfp_service --config <path_to_agent_config> --qsfp-config <path_to_qsfp_config>

* To release a lab switch:

  * `Basset dashboard UI <https://www.internalfb.com/netcastle>`_:
  Select a reserved switch and click the release button.

  * Command line:
  ::

    % basset release -f -y fboss:darwin:darwin.35.01.mpk17

* To test QSFP service by canarying it onto a production switch:
::

    % netwhoami find operating_system=fboss,hw=minipack | head -n 1
    <switch name>
    % fbpkg build -E neteng.fboss.qsfp_service
    <pkg tag>
    % fbossdeploy qsfp-service canary-on <switch name> --tag <pkg tag>

* An example of testing the remediation counter on a production switch by draining this switch and canarying qsfp_service onto it:
::

    % netwhoami find operating_system=fboss,hw=minipack | head -n 1
    fsw002.p035.f01.rva3.tfbnw.net
    % ssh netops@fsw002.p035.f01.rva3.tfbnw.net
    [netops@fsw002.p035.f01.rva3.tfbnw.net ~]$ fboss amidrain
    1 is NOT DRAINED
    % drainer drain fsw002.p035.f01.rva3.tfbnw.net -t T90111204
    [netops@fsw002.p035.f01.rva3.tfbnw.net ~]$ fboss amidrain
    1 is DRAINED
    % buck build @mode/opt --show-output //fboss/qsfp_service:qsfp_service
    % fbossdeploy qsfp-service canary-on --build neteng.fboss.qsfp_service fsw002.p035.f01.rva3.tfbnw.net --skip-drain "already drained"
    [netops@fsw002.p035.f01.rva3.tfbnw.net ~]$ /etc/packages/neteng-fboss-qsfp_service/current/qsfp_service --version

Force a link down and then validate that the remediation counter increments its value:
::

    % [netops@fsw002.p035.f01.rva3.tfbnw.net ~]$ sudo wedge_qsfp_util eth2/1/1 --tx-disable
    % [netops@fsw002.p035.f01.rva3.tfbnw.net ~]$ fboss port state show eth2/1/1

Finally, canary off qsfp_service from this switch and undrain it:
::

    % fbossdeploy qsfp-service canary-off fsw002.p035.f01.rva3.tfbnw.net --skip-drain "already drained"
    % drainer undrain job:3104343&>
    [netops@fsw002.p035.f01.rva3.tfbnw.net ~]$ fboss amidrain
    1 is NOT DRAINED
    [netops@fsw002.p035.f01.rva3.tfbnw.net ~]$ fboss-build-info show

QSFP utility
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* To build QSFP utility:
::

    % cd fbsource/fbcode
    % buck build @mode/opt --show-output //fboss/util:wedge_qsfp_util

    If the build is successful, there will be an output similar to the one below:

    //fboss/util:wedge_qsfp_util buck-out/gen/aab7ed39/fboss/util/wedge_qsfp_util

* To reduce the output binary size:
::

    % strip -g buck-out/gen/aab7ed39/fboss/util/wedge_qsfp_util

* To run QSFP utility on a lab switch, copy the output binary to this switch, and run it:
::

    % scp buck-out/gen/aab7ed39/fboss/util/wedge_qsfp_util root@darwin.35.01.mpk17:/tmp/
    % ssh root@darwin.35.01.mpk17
    % ./wedge_qsfp_util eth1/1/1 --direct-i2c

Unit tests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* To run a unit test (e.g. StatsPublisher, qsfp_util) from a devserver:
::

    % cd fbsource/fbcode
    % buck run //fboss/qsfp_service/facebook/test:stats-publisher-test
    % buck run //fboss/util/test:qsfp_util_test

* To build a unit test (e.g. darwin_i2c_bus_tests) and run it on a switch:
::

    % buck build @mode/opt --show-output //fboss/lib/fpga/facebook/darwin/tests:darwin_i2c_bus_tests
    % strip -g buck-out/gen/aab7ed39/fboss/lib/fpga/facebook/darwin/tests/darwin_i2c_bus_tests
    % rsync buck-out/gen/aab7ed39/fboss/lib/fpga/facebook/darwin/tests/darwin_i2c_bus_tests root@darwin.35.01.mpk17:/tmp/
    % ssh root@darwin.35.01.mpk17
    % ./darwin_i2c_bus_tests

QSFP HW tests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* To run all QSFP HW tests on a devserver:
::

    % netcastle --team fboss_qsfp --basset-query "fboss:darwin:darwin.35.01.mpk17" --test-config "darwin/physdk-credo-0.7.2/credo-0.7.2"

* To build QSFP HW tests and run them on a switch, reserve a switch in basset by picking any free systems from fboss.qsfp.autotest pool that have the attribute "transceivers_connected=yes":
::

    % buck build @mode/opt --show-output //fboss/qsfp_service/test/hw_test:qsfp_hw_test-credo-0.7.2
    % strip -g buck-out/gen/aab7ed39/fboss/qsfp_service/test/hw_test/qsfp_hw_test-credo-0.7.2
    % rsync buck-out/gen/aab7ed39/fboss/qsfp_service/test/hw_test/qsfp_hw_test-credo-0.7.2 root@elbert.03.09.fre101:/tmp/

    Then on the switch:

    % ./qsfp_hw_test-credo-0.7.2 --config agent_cfig --qsfp_config qsfp_cfig

Link tests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* In configerator repo:
::

    % arc build
    % arc canary

* In fbcode repo, build and run link tests:
::

    % netcastle --team fboss_link --basset-query "fboss:darwin:darwin.35.01.mpk17" --test-config "darwin/bcm/asicsdk-6.5.21/6.5.21"
    % arc canary

* To build link tests and run them on a switch:
::

    % buck build @mode/opt-asan --show-output //fboss/agent/test/link_tests:bcm_link_test-6.5.21
    % strip -g buck-out/gen/aab7ed39/fboss/agent/test/link_tests/bcm_link_test-6.5.21
    % rsync buck-out/gen/aab7ed39/fboss/agent/test/link_tests/bcm_link_test-6.5.21 root@darwin.35.01.mpk17:/tmp/

    Then on the switch, manually start qsfp_service on the switch, and then run link tests:

    % ./bcm_link_test-6.5.21 --config <agent_config_path>
