.. fb:display_title::
  Sai Replayer


As FBOSS wedge agent is moving towards SAI switch, a tool is needed to log SAI API calls and recreate certain problematic ASIC states.
Such “replaying” of the issue would help joint Facebook-vendor debugging when an error occurs in FBOSS production environment,
but vendors are unable to reproduce it from the logs. Before SAI implementation, FBOSS had `BcmCinter
<https://www.internalfb.com/intern/diffusion/FBS/browse/master/fbcode/fboss/agent/facebook/wiki/bcmcinter.rst>`_
to log Broadcom API calls and used bcm user shell to program the ASIC. The usefulness of BcmCinter made us realize a similar mechanism is needed for SAI implementation.


As a successor of BcmCinter, SAI Replayer is designed to be vendor-agnostic.
It captures all SAI APIs at runtime and auto-generates C++ code that emulates the API calls.
The code can then be compiled into a binary and directly executed on the switch to recreate the problematic ASIC state.
The readable C++ code would also help us understand the details of SAI API calls at runtime.

Example of the generated code
--------------------------------

.. code-block:: c++

  sai_attributes[0].id = 0;
  sai_attributes[1].id = 1;
  sai_attributes[2].id = 2;
  sai_attributes[0].value.oid = aclTableGroup_0;
  sai_attributes[1].value.oid = aclTable_0;
  sai_attributes[2].value.u32 = 23;
  sai_object_id_t aclTableGroupMember_0;
  status = acl_api->create_acl_table_group_member(&aclTableGroupMember_0, switch_0, 3, sai_attributes);
  if (status != 0) printf("Unexpected rv at 3 with status %d\n", status);


Getting started with SAI Replayer
-----------------------------------

Flags and different modes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enable logging for SAI Replayer, simply pass the flag ``--enable_replayer`` to the wedge agent binary or test binary.
Once enabled, the generated C++ code can be found under ‘/var/facebook/logs/fboss/sai_replayer.log’, which can be compiled using buck build.

Logging for ‘send_hostif_packet’ API is disabled by default for performance concern, due to the overhead for logging each sent packet to the dataplane.
However, if it is needed for certain scenarios, such as the verification test for port blackholing,
use the flag ``--enable_packet_log`` to log the send packet API
(be mindful that by enabling this flag, logging would introduce significant I/O overhead at runtime and a much larger SAI Replayer code).


Compilation with buck
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SAI Replayer generates C++ code by default. It is used to build buck target since main and helper functions are already defined in `Main.cpp
<https://www.internalfb.com/intern/diffusion/FBS/browsefile/master/fbcode/fboss/agent/hw/sai/tracer/run/Main.cpp>`_.
Take the generated code and replace the content in `SaiLog.cpp
<https://www.internalfb.com/intern/diffusion/FBS/browse/master/fbcode/fboss/agent/hw/sai/tracer/run/SaiLog.cpp>`_.
Then build the buck target, depending on the Sai implementation and sdk version - ‘//fboss/agent/hw/sai/tracer/run:sai_replayer-{impl}-{sdk_version}’.

For example, use the following command for broadcom sai implementation.

.. code-block:: sh

  export sdkVersion=brcm-08.08.2020_odp
  buck build @mode/dbg //fboss/agent/hw/sai/tracer/run:sai_replayer-${sdkVersion}

The built binary can be copied to the test switch to replay SAI API calls. See appendix for building Sai Replayer code in C with gcc.

For example,

.. code-block:: sh

  export testSwitch=rsw1at.12.prn3
  scp buck-out/gen/fboss/agent/hw/sai/tracer/run/sai_replayer-${sdkVersion} root@$testSwitch

Running the executable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Simply run the executable to replay the generated code. For example,

.. code-block:: sh

  export sdkVersion=brcm-08.08.2020_odp 
  root@$testSwitch ./sai_replayer-${sdkVersion}


At each API call, the generated code will check whether the return status is the same as the one at runtime.

.. code-block:: c++

  if (status != 0) printf("Unexpected rv at 3 with status %d \n", status);

If the return status is not the same, it will print out this message with the sequence number of the API call
so that users can look at the generated code and debug from there.


Adding support for new APIs & Attributes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As more SAI APIs and attributes in `SAI spec <https://github.com/opencomputeproject/SAI/tree/master/inc>`_ are supported in FBOSS,
these APIs and attributes will also need to be added to SAI Replayer logging.
Otherwise, the generated code produced by SAI Replayer will be incomplete and thus unable to faithfully recreate the ASIC state.


New attributes for existing APIs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Adding new attributes to existing APIs should be fairly straightforward. One of the examples would be the ``setSwitchAttributes`` method in `SwitchApiTracer
<https://www.internalfb.com/intern/diffusion/FBS/browsefile/master/fbcode/fboss/agent/hw/sai/tracer/SwitchApiTracer.cpp>`_.
Add new attribute ids in the switch statement and then invoke helper method in `Utils.h
<https://www.internalfb.com/intern/diffusion/FBS/browsefile/master/fbcode/fboss/agent/hw/sai/tracer/Utils.h>`_ to generate corresponding code of the type.


For example, if the new attribute is of type ``sai_int32_t``, add the following code for the new attribute

.. code-block:: c++

  case OLD_ATTRIBUTE:
      ...
      break;
  case NEW_ATTRIBUTE:
      attrLines.push_back(s32Attr(attr_list, i));
      break;

Or the new attribute is a list of sai object ids,

.. code-block:: c++

  case NEW_ATTRIBUTE:
      oidListAttr(attr_list, i, listCount++, attrLines);
      break;


Most of the Sai types are supported by helper methods defined in Utils.h.
If there’s no helper method for a new Sai type, please add it to Utils.h/.cpp and make sure the generated code is correct.

New APIs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Adding new APIs would be slightly more complicated compared to new attributes,
but the steps are well-defined and we have sufficient examples (please see D22495021 for supporting Buffer API logging).
The naming and variables might be different due to refactoring, but the general idea is the same.

* Create NewApiTracer.h and declare the following functions

  * ``wrappedNewApi()``, which returns a struct of function pointers for the wrapped functions (See Design Choices & Philosophy for more details)
  * ``setNewObjectAttributes()``, which invokes corresponding helper functions depending on the attribute type. There could be multiple ``setAttributes()`` methods, depending on the number of objects managed by this API. (e.g. BufferPool and BufferProfile for BufferAPI)

* Create NewApiTracer.cpp and implement the functions above. The ``wrap_real_functions()`` declared locally will make the real API call and then invoke log functions in SaiTracer to generate C code for the call.

* Add NewApiTracer.cpp to TARGETS and to OSS cmake file AgentHwSaiTracer.cmake.

* In SaiTracer.h,

  * Create a new variable ``sai_new_api_t* newApi_`` to store the struct of function pointers returned by sai_api_query().
  * Add ``SAI_OBJECT_TYPE_NEW_OBJECT`` to ``varNames_`` for variable names.
  * Add ``SAI_OBJECT_TYPE_NEW_OBJECT`` to ``fnPrefix_`` to specify which API name to call for this object type’s operation.

* In SaiTracer.cpp,

  * Include NewApiTracer.h
  * In ``__wrap_sai_api_query()``, add a case statement for ``SAI_API_NEW``.
  * In ``setAttrList()``, call ``setNewObjectAttributes()`` previously implemented in NewApiTracer.cpp.
  * Add ``SAI_OBJECT_TYPE_NEW_OBJECT`` to ``varCounts_``.

* Add helper methods to Utils.h/.cpp if needed.


Verification
-----------------------------------

In order to verify the generated code reproduces the ASIC state as expected, we use the warmboot hardware tests to verify the correctness of SAI Replayer.

Verification Tests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Run the hardware tests with ``--setup_for_warmboot`` flag. It will setup the software state and SAI Replayer will generate C++ code to reproduce the ASIC state.

2. Compile the generated C++ code into binary and run it on the switch. This should overwrite the ASIC state from the first step using the generated code.

3. Run the hardware tests again, and it should do a warmboot and pick up the software state from the first step and the ASIC state from the second step. If it passes the test case, we verify the ASIC state is setup as expected.

For example,

.. code-block:: sh

  export sdkVersion=brcm-08.08.2020_odp
  export testSwitch=rsw1at.12.prn3

  # Build and copy SAI test to test switch
  cd /data/users/$(whoami)/fbsource/fbcode
  buck build @mode/dbg //fboss/agent/hw/sai/hw_test:sai_test-${sdkVersion}
  rsync -avz --progress buck-out/gen/fboss/agent/hw/sai/hw_test/sai_test-${sdkVersion} root@$testSwitch:/root

  # Run test to generate replay log
  ssh root@$testSwitch ./sai_test-${sdkVersion} --config /root/wedge100_alpm.agent.conf --gtest_filter=HwVlanTest.VlanApplyConfig --enable-replayer --sai_log=/tmp/sai_log.c --setup-for-warmboot

  # Copy the log to devserver and build
  rsync -avz --progress root@$testSwitch:/tmp/sai_log.c fboss/agent/hw/sai/tracer/run/SaiLog.cpp
  buck build @mode/dbg //fboss/agent/hw/sai/tracer/run:sai_replayer-${sdkVersion}

  # Copy the built log file and run
  rsync -avz --progress buck-out/gen/fboss/agent/hw/sai/tracer/run/sai_replayer-${sdkVersion} root@$testSwitch:/root
  ssh root@$testSwitch ./sai_replayer-${sdkVersion}

  # Clean up generated SaiLog.cpp
  hg revert fboss/agent/hw/sai/tracer/run/SaiLog.cpp

.. fb:px:: 1hxNJ

NetCastle job
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The verification tests are automated by the NetCastle `sai_replayer_test <https://fburl.com/tests/ra08snjb>`_. Specifically, it does the following steps

1. Run Sai tests on switch with ``--setup_for_warmboot`` and ``--enable_replayer`` flags to generate C++ code for the test.

2. Download the generated code from switch to vm and compile the executable using buck build.

3. Upload the executable to switch and run the executable.

4. Run Sai tests with warmboot to verify the test is passing.

Several representative tests will be enabled on-diff to check whether new changes are breaking the sai_replayer_test. Other tests will be run as continuous jobs.
To monitor the tests, see `testX <https://fburl.com/tests/ra08snjb>`_ for more details.


Design Choices & Philosophy
-----------------------------------

Linker Wrap and Wrapped Function Pointers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The original design of SAI Replayer was to use linker wrap to intercept SAI API calls and then perform logging in the wrap function.
However, we soon realized that the ``sai_api_query()`` returns a struct of function pointers instead of well-defined functions in the `interface
<https://github.com/opencomputeproject/SAI/blob/master/inc/saiacl.h#L3216>`_. Linker is not able to wrap these function pointers returned at runtime.

.. fb:px:: 1hxPt

Therefore, we use a combination of link wrapper and wrapped function pointers to enable SAI API logging.
First, ``sai_api_query()`` is intercepted by the linker wrap, and before giving FBOSS the real function pointers from SAI implementation,
we replace those function pointers with locally defined wrapped functions, such as


.. code-block:: c++

  sai_status_t wrap_create_acl_table(
      sai_object_id_t* acl_table_id,
      sai_object_id_t switch_id,
      uint32_t attr_count,
      const sai_attribute_t* attr_list) {
    auto rv = SaiTracer::getInstance()->aclApi_->create_acl_table(
        acl_table_id, switch_id, attr_count, attr_list);

    SaiTracer::getInstance()->logCreateFn(
        "create_acl_table",
        acl_table_id,
        switch_id,
        attr_count,
        attr_list,
        SAI_OBJECT_TYPE_ACL_TABLE,
        rv);
    return rv;
  }


The function pointer returned to FBOSS will be ``&wrap_create_acl_table`` instead of the real function pointers.
When FBOSS calls ``create_acl_table``, it actually invokes ``wrap_create_acl_table``,
which will make the real create call to SAI implementation and then generate code to replay that call.
New wrapped functions are needed when FBOSS is calling into new APIs of SAI implementation.


Code Structure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
To separate the core codegen logic from API-specific logic, files under SAI Replayer is structured as follows:

* The core codegen logic lives in SaiTracer.cpp - it manages the linker wrap for ``sai_api_query()`` and replaces the real function pointers with wrapped function pointers. It also provides common methods such as ``logCreateFn()`` that can be used to generate code for most of the Create APIs (There are also different methods for special types including entry types and switch since these types are slightly different from the common ones).

* All API-specific logic lives in ObjectApiTracer.cpp - it defines wrapped functions for each SAI API call and returns those function pointers to the ``sai_api_query()`` call. It also handles the attribute setup for different attribute types (See AclTable `example <https://fburl.com/diffusion/382b89n9>`_.

* Utils.cpp provides helper methods to serialize and generate code for attribute types, such as objectIds, int types as well as int lists. These methods are mostly interacting values within the object and concatenate strings to produce properly formatted code.

* AsyncLogger.cpp manages logging for the optimization covered in the next section. It provides non-blocking logging for code generated at runtime.


Async Logger optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
See wiki for `Async Logger <https://www.internalfb.com/intern/wiki/Fboss/wiki/async_logger/>`_.


Appendix
-----------------------------------

Compile generated code with GCC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sai Replayer by default generates C++ code that is compiled using buck. However, if buck is not available or C code is absolutely needed,
the code within ``run_tracer()`` is also C compatible with a main function. It then can be linked with necessary libraries and compiled using gcc.
Unlike buck targets where third-party libraries are updated automatically, please make sure the third-party libraries are up-to-date when compiled using gcc
(use ‘git pull’ to update the tp2’s directories). Here’s the list of libraries needed to compile the binary for broadcom Sai:

* libm
* libpthread
* librt
* libstdc++
* libdl
* Directory containing all sai headers (e.g. sai/1.6.0/platform007/ca4da3d/include/)
* libsai (e.g. brcm-sai/4.2.0.7_odp/platform007/331835f/lib/libsai.a)
* libxgs_robo (e.g. broadcom-xgs-robo/6.5.18/platform007/1b1141c/lib/libxgs_robo.a)
* libprotobuf (e.g. protobuf/3.7.0/platform007/fbc192b/lib/libprotobuf.a)

Use the following command to build the binary (and substitute correct library path):

.. code-block:: sh

  export sai_headers=sai/1.6.0/platform007/ca4da3d/include/
  export sai_lib=brcm-sai/4.2.0.7_odp/platform007/331835f/lib/libsai.a
  export brcm_lib=broadcom-xgs-robo/6.5.18/platform007/1b1141c/lib/libxgs_robo.a
  export protobuf_lib=protobuf/3.7.0/platform007/fbc192b/lib/libprotobuf.a

  gcc sai_log.c -I $sai_headers -lm -lpthread -lrt -lstdc++ -ldl $sai_lib $brcm_lib $protobuf_lib

Running the executable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
After compiling the executable, copy it to the test switch and replay the logged SAI APIs.
One thing to notice is that the executable generated by gcc needs a specific linker library and library path (because switches are running Centos 7 instead of 8).
Use the following command to run the gcc executable:

.. code-block:: sh

  export lib_path=/usr/local/fbcode/platform007/lib
  $lib_path/ld-linux-x86-64.so.2 --library-path $lib_path ./a.out
