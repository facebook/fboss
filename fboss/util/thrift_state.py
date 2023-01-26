import argparse
import json

from neteng.fboss.switch_state.types import WarmbootState
from thrift.py3 import deserialize, Protocol, serialize


def main():
    psr = argparse.ArgumentParser(
        description="This script reads and/or modify agent thrift state, which is dumped during warmboot exit."
    )
    psr.add_argument("--agent_state", required=True, type=str)
    psr.add_argument("--output", required=False, default=None, type=str)
    args = psr.parse_args()

    with open(args.agent_state, "rb") as agent_state_file:
        data = agent_state_file.read()
        thrift_obj = deserialize(
            structKlass=WarmbootState,
            buf=data,
            protocol=Protocol.BINARY,
        )
        json_obj = json.loads(serialize(thrift_obj, Protocol.JSON))
        if args.output:
            with open(args.output, "w") as output_file:
                json.dump(json_obj, fp=output_file, indent=2, sort_keys=True)
        else:
            print(
                json.dumps(
                    json_obj,
                    indent=2,
                    sort_keys=True,
                )
            )


if __name__ == "__main__":
    main()
