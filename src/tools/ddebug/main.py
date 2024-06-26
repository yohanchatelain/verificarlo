import ctypes
import ctypes.util
import os
import sys

from verificarlo.ddebug import DD_exec_stat, DD_stoch, dd_config


class DDline(DD_stoch.DDStoch):
    def __init__(self, config, prefix="dd.line"):
        DD_stoch.DDStoch.__init__(self, config, prefix)

    def referenceRunEnv(self):
        return {
            "VFC_BACKENDS": "libinterflop_ieee.so",
            "VFC_DDEBUG_GEN": os.path.join(self.ref_, "dd.line.%%p"),
        }

    def isFileValidToMerge(self, name):
        return name.startswith("dd.line.")

    def getDeltaFileName(self):
        return "dd.line"

    def sampleRunEnv(self, dirName):
        return {
            "VFC_DDEBUG_INCLUDE": os.path.join(
                dirName, self.getDeltaFileName() + ".include"
            )
        }

    def coerce(self, delta_config):
        return "\n".join([line[:-1] for line in delta_config])


def disable_ASLR():
    # We call personality(ADDR_NO_RANDOMIZE) to disable ASLR, so that addresses during reference run
    # always match addresses during sample runs (even for .so code).
    ADDR_NO_RANDOMIZE = 0x0040000
    libc_name = ctypes.util.find_library("c")
    libc = ctypes.CDLL(libc_name)
    personality = libc.personality
    personality(ADDR_NO_RANDOMIZE)


def main():
    disable_ASLR()
    et = DD_exec_stat.exec_stat("dd.line")
    config = dd_config.ddConfig(sys.argv, os.environ)
    dd = DDline(config)
    dd.run()
    et.terminate()


if __name__ == "__main__":
    main()
