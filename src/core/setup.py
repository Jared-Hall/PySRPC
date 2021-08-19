import sysconfig
from distutils.core import setup, Extension
module1 = Extension("srpc",
                    include_dirs = ["/usr/local/include"],
                    libraries = ["srpc", "ADTs"],
                    library_dirs = ["/usr/local/lib"],
                    sources = ["pysrpc.c"])

setup(name = "SRPC",
        version = '0.2',
        description = "A Python extension for SRPC",
        author = "Jared W. Hall",
        author_email = "jhall10@uoregon.edu",
        ext_modules = [module1])
