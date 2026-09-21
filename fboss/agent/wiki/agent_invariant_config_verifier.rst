Agent Invariant Config Verifier
-------------------------------


Problem
-------

There are subsets of unique configuration transitions across different platforms in the fleet.
We need to ensure that these configuration transitions are safe for DC push. FBOSS config testing is part of the effort to
validate these transitions before they’re rolled out to production.

Solution
--------

Agent Invariant Config Verifier is a tool that allow us to automate the process of verifying these configuration transitions,
as well as sdk and commit-id transitions. This tool will allow us to verify transitions before they roll out to production.
These transitions are tested by ProdInvariantTests which ensure key invariants, or features, does not break through upgrades.

The tool works as follow:
    - Discover all unique config transitions per platforms using fboss-config-stager unique-config-transitions
    - With each unique transition, create a Thrift Client to fetch prod to trunk configurations and save it temporary on disk.
    - Run ProdInvariantTests via netcastle and pass in the desired sdk, commit, and configuration transitions.
    - Capture the netcastle output to produce PASS/FAIL signal.
