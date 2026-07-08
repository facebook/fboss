# pyre-unsafe

import argparse


class CliWrapper:
    def __init__(self, name, description, options=None):
        self.name = name
        self.description = description
        self.options = options

    def __call__(self, func):
        def wrapper():
            parser = argparse.ArgumentParser(
                prog=self.name,
                description=self.description,
            )
            # pyrefly: ignore [missing-attribute]
            for option, (desc, _) in self.options.items():
                parser.add_argument(f"--{option}", help=desc, action="store_true")
            args = dict(vars(parser.parse_args()))
            while len(args) > 0:
                key, value = args.popitem()
                if not value:
                    continue
                key = key.replace("_", "-")
                # pyrefly: ignore [unsupported-operation]
                self.options[key][1]()
            return func()

        return wrapper
